Issue Submission Guidelines
---------------------------

While the Tekdaqc Firmware collaborators are very happy to assist and provide support for the library, they are all doing this at the expense of other development or on their own time. So before you post, please read the guidelines below.
* When submitting an issue, please make sure it is related to the Tekdaqc Firmware or its library. Tenkiv has published several repositories related to the Tekdaqc so please take the time to ensure you are posting an issue against the correct one. Additionally, please do not use the issue tracker as a support forum. Please direct such questions to either our [Intelligent Automation, Computer Interface, & DAQ Community](https://plus.google.com/u/0/communities/109351353187504550254) on [![DAQ Community on Google Plus](https://ssl.gstatic.com/images/icons/gplus-16.png)](https://plus.google.com/u/0/communities/109351353187504550254) or better yet, to [Stack Overflow](http://www.stackoverflow.com/) and share the question with the community.
* Search the Issues list before you submit an issue. If you are new to the Tekdaqc frameworks, chances are, somebody asked a similar question before you did. We often notice that same questions that have been answered before get asked over and over again.
* When submitting an issue, please be as constructive as possible. Non-constructive questions such as merely asking for more examples will be viewed as asking us to do your work for you. These issues will be closed immediately by our collaborators. Issues which do not have sufficient information to diagnose, we will ask for clarification. If we do not receive clarification within 10 days, we will close the issue.
* If you think there is a bug in the library, please provide exact details to reproduce the bug and stack traces (in case of runtime exceptions) when submitting an issue.
* Please use Markdown to format your issue and to properly format your code. This ensures better readability for others which helps to receive better support. Hard-to-understand submission resulting from poorly formatted post may result in immediate closure by our collaborators.
* The Tekdaqc repositories are documented and supported in the English language. Please submit your issues written in comprehensive English so that our collaborators and others around the world may be able to easily read and understand what your issue is. Submissions written in any other languages or in very uncomprehensive English will be closed if not corrected. If English is not your native tongue, please use a translator and we will do our best to accommodate.
* If there are no activity within the issue thread for 10 days, it will be deemed stale by our collaborators and will be closed. You may re-open the issue after 10 days if you need further help. When the issue is solved, please mention whether the issue was helpful (it would also be nice to thank whoever helped solve the issue) and close the issue.

Pull Request Guidelines
-----------------------

* Changes should be tested.
* Affected functions should have accurate documentation. We will use this to update the API docs.
* Affected documentation, such as tutorials, should be clearly noted in the pull request description so that the collaborators can update them.
* Source code formatting should be hygienic:
  * No trailing or inconsistent whitespace.
  * No exceptionally long lines.
  * Consistent placement of brackets.
  * ...etc.
  * _Hint: Use Eclipse's auto-formatter on affected files (but please commit separately from functional changes)._
* Commit history should be relatively hygienic:
  * _Roughly_ one commit per logical change.
  * Log messages are clear and understandable.
  * Few (or no) "merge" commits.
  * _Hint: `git rebase -i` is great for cleaning up a branch._
  * We DO NOT require a single commit per pull request, however we will ask you to clean up your pull request if it is excessively cluttered or broad.

Pull requests for branches that are still in development should be prefixed with `WIP:` so that they don't get accidently merged. Remove `WIP:` once the pull request is considered finalized and ready to be reviewed and merged.

License
-------

By contributing code or issues to the Tekdaqc Android Library, you are agreeing to release it under the [Apache License, Version 2.0](http://opensource.org/licenses/Apache-2.0).
