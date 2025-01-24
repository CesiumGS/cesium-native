# Contribution Guide {#contributing}

<!--! \cond DOXYGEN_EXCLUDE !-->

## Table of Contents

- [📬Submitting an Issue](#submitting-an-issue)
- [📝Opening a Pull Request](#opening-a-pull-request)
  - [Contributor License Agreement (CLA)](#contributor-license-agreement-cla)
  - [Pull Request Guidelines](#pull-request-guidelines)
- [⚖️Code of Conduct](#️code-of-conduct)
  
<!--! \endcond -->

Community involvement in Cesium Native has been and continues to be a key part of its development. Contributing to Cesium Native can take many forms:
<!--! [TOC] -->

- Making a pull request to add features or fix issues.
- Submitting an issue to report a bug.
- Triaging existing issues. This can include attempting to reproduce issues or sharing additional information you have.
- Being active on the [Cesium community forum](https://community.cesium.com/) by answering questions and providing input on Cesium's direction.
- Writing tutorials, creating examples, and improving the reference documentation.
- Sharing projects you've made with Cesium Native with us through the [Cesium community forum](https://community.cesium.com/) or at hello@cesium.com.

More details about writing issues and pull requests is detailed below. For any other guidance you need, don't hesitate to ask on the [Cesium community forum](https://community.cesium.com/)!

## 📬Submitting an Issue

If you have a question, please do not submit an issue; instead, search the [Cesium community forum](https://community.cesium.com/). The forum is very active and there are years of informative archives, often with answers from the core Cesium team. If you do not find an answer to your question, start a new thread and you'll likely get a quick response!

If you think you've found a bug in Cesium Native, first search the [issues](https://github.com/CesiumGS/cesium-native/issues). If an issue already exists, please add a comment expressing your interest and any additional information. This helps us prioritize issues.

If a related issue does not exist, submit a new one. Please be concise and include as much of the following information as is relevant:

- Minimum amount of sample code (and data).
- Screenshot or animated .gif if appropriate (try [LICEcap](http://www.cockos.com/licecap/)). For example, see [#803](https://github.com/CesiumGS/cesium-native/issues/803). Screenshots are particularly useful for exceptions and rendering artifacts.
- Link to the thread if this was discussed on the Cesium forum or elsewhere. For example, see [#878](https://github.com/CesiumGS/cesium-native/issues/878).
- Your operating system and version, compiler and version, and video card. If you're using Cesium Native with an engine like Unreal or Unity, please include this information as well. Are they all up-to-date? Is the issue specific to one of them?
- The version of Cesium Native. Did this work in a previous version?
- Ideas for how to fix or workaround the issue. Also mention if you are willing to help fix it. If so, the Cesium team can often provide guidance and the issue may get fixed more quickly with your help.

## 📝Opening a Pull Request

Pull requests are a huge help in the development of Cesium Native. Following the tips in this guide will help your pull request get merged quickly.

> If you plan to make a major change, please start a new thread on the [Cesium community forum](https://community.cesium.com/) first. Pull requests for small features and bug fixes can generally just be opened without discussion on the forum.

### Contributor License Agreement (CLA)

Before we can review a pull request, we require a signed Contributor License Agreement. There is a CLA for:

- [individuals](https://docs.google.com/forms/d/e/1FAIpQLScU-yvQdcdjCFHkNXwdNeEXx5Qhu45QXuWX_uF5qiLGFSEwlA/viewform) and
- [corporations](https://docs.google.com/forms/d/e/1FAIpQLSeYEaWlBl1tQEiegfHMuqnH9VxyfgXGyIw13C2sN7Fj3J3GVA/viewform).

This only needs to be completed once, and enables contributions to all of the projects under the [CesiumGS](https://github.com/CesiumGS) organization, including Cesium Native. The CLA ensures you retain copyright to your contributions, and provides us the right to use, modify, and redistribute your contributions using the [Apache 2.0 License](LICENSE).

If you have any questions, feel free to reach out to hello@cesium.com!

### Pull Request Guidelines

Our code is our lifeblood so maintaining Cesium Native's high code quality is important to us.
- For an overview of our workflow see [github pull request workflows](https://cesium.com/blog/2013/10/08/github-pull-request-workflows/).
- Pull request tips
  - If your pull request fixes an existing issue, include a link to the issue in the description (like this: &quot;Fixes [#1](https://github.com/CesiumGS/cesium-native/issues/1)&quot;). Likewise, if your pull request fixes an issue reported on the Cesium forum, include a link to the thread.
  - If your pull request needs additional work, include a [task list](https://github.com/blog/1375%0A-task-lists-in-gfm-issues-pulls-comments).
  - Once you are done making new commits to address feedback, add a comment to the pull request such as `"this is ready"`.
- Code and tests
  - Review the [C++ Style Guide](doc/topics/style-guide.md). These guidelines help us write consistent, performant, less buggy code and improve our productivity by standardizing the decisions we make across the codebase.
  - Verify that all tests pass, and write new tests with excellent code coverage for new code. Tests can be built and run using the `cesium-native-tests` target.
  - Update [CHANGES.md](CHANGES.md) with a brief description of your changes.
  - If you plan to add a third-party library, start a [GitHub issue](https://github.com/CesiumGS/cesium/issues/new) discussing it first.

## ⚖️Code of Conduct

To ensure an inclusive community, contributors and users in the Cesium community should follow the [code of conduct](https://github.com/CesiumGS/cesium/blob/main/CODE_OF_CONDUCT.md).