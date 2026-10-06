/**
 * Tags a cesium-native release and fast-forwards the cesium.com branch to it,
 * implementing the "Release" and "Publish Updated Reference Docs" steps in
 * doc/topics/release-process.md.
 *
 * Run it like `npm run publish-release -- 0.65.0 --dry-run`.
 */

const { spawnSync } = require("node:child_process");
const fs = require("node:fs");
const path = require("node:path");
const readline = require("node:readline/promises");

// Always operate on the repository root, no matter where the script is invoked from
const rootDir = path.resolve(__dirname, "..", "..");
process.chdir(rootDir);

// Defaults, overridable by the options below
const docsBranch = "cesium.com";
const sourceBranch = "main"; // generally should start from main
let remote = "origin";
let dryRun = false;
let assumeYes = false;
let version = "";

function usage(log = console.log) {
  log(
    `Usage: npm run publish-release -- <version> [--dry-run] [--yes] [--remote <name>]

Arguments:
  <version>        Release version without the leading "v" (e.g. 0.65.0).

Options:
  --dry-run        Print every mutating command without running it.
  --yes            Skip the confirmation prompt before pushing.
  --remote <name>  Git remote to use (default: origin).
  -h, --help       Show this help.`,
  );
}

// Print an error and stop the script
function fail(message, exitCode = 1) {
  console.error(`error: ${message}`);
  process.exit(exitCode);
}

// Inspect arguments
const args = process.argv.slice(2);
while (args.length > 0) {
  const arg = args.shift();
  if (arg === "--dry-run") {
    dryRun = true;
  } else if (arg === "--yes") {
    assumeYes = true;
  } else if (arg === "--remote") {
    remote = args.shift();
    if (remote === undefined) {
      fail("--remote requires a value", 2);
    }
  } else if (arg === "-h" || arg === "--help") {
    usage();
    process.exit(0);
  } else if (arg.startsWith("-")) {
    console.error(`error: unknown option: ${arg}`);
    usage(console.error);
    process.exit(2);
  } else if (version !== "") {
    fail(`unexpected argument: ${arg}`, 2);
  } else {
    version = arg;
  }
}

// Check that version is supplied as a required field
if (version === "") {
  console.error("error: version is required");
  usage(console.error);
  process.exit(2);
}

// Check version format
if (!/^\d+\.\d+\.\d+$/.test(version)) {
  fail(`version must look like 0.65.0, got: ${version}`, 2);
}

const tag = `v${version}`;

// Runs a git command and returns its trimmed stdout, or undefined if it failed.
function git(...gitArgs) {
  const result = spawnSync("git", gitArgs, { encoding: "utf8" });
  if (result.error !== undefined || result.status !== 0) {
    return undefined;
  }
  return result.stdout.trim();
}

// Runs a git command only to find out whether it succeeds.
function gitSucceeds(...gitArgs) {
  return spawnSync("git", gitArgs, { stdio: "ignore" }).status === 0;
}

// Quoting is only cosmetic here, so that echoed commands can be pasted into a shell.
function quoteForDisplay(arg) {
  return /^[\w./:=-]+$/.test(arg) ? arg : `'${arg.replace(/'/g, `'\\''`)}'`;
}

// Echoes a mutating command, and runs it unless --dry-run was passed.
function runCommand(...commandArgs) {
  console.log(`+ ${commandArgs.map(quoteForDisplay).join(" ")}`);
  if (dryRun) {
    return;
  }
  const result = spawnSync(commandArgs[0], commandArgs.slice(1), {
    stdio: "inherit",
  });
  if (result.error !== undefined) {
    fail(`failed to run ${commandArgs[0]}: ${result.error.message}`);
  }
  if (result.status !== 0) {
    fail(`command failed: ${commandArgs.join(" ")}`);
  }
}

// Remember the starting branch so it can be restored later, since the release process switches branches to cesium.com
const originalBranch = git("symbolic-ref", "--quiet", "--short", "HEAD");

// If something fails, put you back on your original branch
function restoreBranch() {
  if (dryRun || originalBranch === undefined) {
    return;
  }
  const current = git("symbolic-ref", "--quiet", "--short", "HEAD");
  if (current !== undefined && current !== originalBranch) {
    console.log(`restoring branch ${originalBranch}`);
    git("checkout", "--quiet", originalBranch);
  }
}
// Runs on every exit path, including failures, to put you back on the original branch
process.on("exit", restoreBranch);
// Without this, Ctrl-C would terminate the process without firing the "exit" handler
process.on("SIGINT", () => process.exit(130));

// Check we are in a git repo and the requested remote exists
if (git("rev-parse", "--git-dir") === undefined) {
  fail(`not a git repository: ${rootDir}`);
}
const remoteUrl = git("remote", "get-url", remote);
if (remoteUrl === undefined) {
  fail(`remote '${remote}' does not exist`);
}

// Check we are releasing from the canonical repo and not a fork
if (!remoteUrl.includes("CesiumGS/cesium-native")) {
  fail(
    `remote '${remote}' is ${remoteUrl}, which is not CesiumGS/cesium-native`,
  );
}

// Check we are on the correct branch for release
if (originalBranch !== sourceBranch) {
  fail(
    `must be run from '${sourceBranch}', currently on '${originalBranch ?? "detached HEAD"}'`,
  );
}

// Check there is no uncommitted work that could follow us across branches
if (git("status", "--porcelain") !== "") {
  fail("working tree is dirty; commit or stash changes before releasing");
}

// Get the latest branches and tags so the checks below compare against current state
console.log(`fetching ${remote}`);
if (!gitSucceeds("fetch", "--quiet", remote, "--tags")) {
  fail(`failed to fetch from ${remote}`);
}

// Check both branches we rely on exist on the remote
for (const branch of [sourceBranch, docsBranch]) {
  if (
    !gitSucceeds(
      "rev-parse",
      "--verify",
      "--quiet",
      `refs/remotes/${remote}/${branch}`,
    )
  ) {
    fail(`${remote}/${branch} not found`);
  }
}

// Check the release commit has already been pushed
const localHead = git("rev-parse", "HEAD");
const remoteHead = git("rev-parse", `${remote}/${sourceBranch}`);
if (localHead !== remoteHead) {
  fail(
    `${sourceBranch} is not in sync with ${remote}/${sourceBranch}; pull or push first`,
  );
}

// Check this version has not already been released
if (gitSucceeds("rev-parse", "--verify", "--quiet", `refs/tags/${tag}`)) {
  fail(`tag ${tag} already exists locally`);
}
if (gitSucceeds("ls-remote", "--exit-code", "--tags", remote, `refs/tags/${tag}`)) {
  fail(`tag ${tag} already exists on ${remote}`);
}

// The release metadata must already be committed by the prepare-release step.
// Check package.json version matches
const packageVersion = JSON.parse(
  fs.readFileSync("package.json", "utf8"),
).version;
if (packageVersion !== version) {
  fail(`package.json version is '${packageVersion}', expected '${version}'`);
}

// Check the fast-forward will be possible, before anything has been pushed
if (
  !gitSucceeds("merge-base", "--is-ancestor", `${remote}/${docsBranch}`, "HEAD")
) {
  fail(
    `${remote}/${docsBranch} is not an ancestor of ${sourceBranch}; a fast-forward merge is not possible`,
  );
}

// Summarize what is about to happen
console.log();
console.log(`repository:   ${remoteUrl}`);
console.log(`tag:          ${tag} at ${localHead}`);
console.log(`docs branch:  ${docsBranch} -> ${tag}`);
console.log(
  dryRun
    ? "mode:         dry run (no changes will be made)"
    : "mode:         live (tags and pushes will be created)",
);
console.log();

async function main() {
  // Last chance to back out before anything is pushed
  if (!dryRun && !assumeYes) {
    const rl = readline.createInterface({
      input: process.stdin,
      output: process.stdout,
    });
    const reply = await rl.question(`Tag and push ${tag} to ${remote}? [y/N] `);
    rl.close();
    if (reply.trim().toLowerCase() !== "y") {
      fail("aborted by user");
    }
  }

  // Release: tag the release commit and publish the tag
  runCommand("git", "tag", "-a", tag, "-m", `${version} release`);
  runCommand("git", "push", remote, tag);

  // Publish updated reference docs: get onto cesium.com, creating it locally if needed
  if (gitSucceeds("rev-parse", "--verify", "--quiet", `refs/heads/${docsBranch}`)) {
    runCommand("git", "checkout", docsBranch);
    runCommand("git", "pull", "--ff-only", remote, docsBranch);
  } else {
    runCommand("git", "checkout", "-b", docsBranch, "--track", `${remote}/${docsBranch}`);
  }

  /* The `--ff-only` behavior ensures that no new commits are created in the process. 
     * We only want to bring over commits from the release tag. If the fast-forward or push fails, 
     * something unusual has probably happened with the git history and it warrants a little investigation before proceeding.
     */
  runCommand("git", "merge", tag, "--ff-only");
  runCommand("git", "push", remote, docsBranch);
  runCommand("git", "checkout", sourceBranch);

  console.log();
  console.log(
    dryRun
      ? "dry run complete; no changes were made"
      : `released ${tag} and updated ${docsBranch}`,
  );
}

main().catch((error) => fail(error.message));
