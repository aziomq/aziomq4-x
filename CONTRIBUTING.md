# Contribution Policy -

If you are familiar with ZeroMQ's process, there should be few surprises here.

Development is done on Github. Ideally the process follows the ZeroMQ community's
[C4] (http://rfc.zeromq.org/spec:22) process guidelines (see licensing notes
below though). The high points, in particular are:

* Log an issue (Github only) that explains the problem you are solving.

* Provide a test case, unless absolutely impossible.

* If you are making a change to the stable version, an issue and test case
are required.

* Make your change as a pull request (Github only, see guidelines below)

* Close the issue when your pull request is merged and the test case passes.

## Contribution guidelines -

### Separate Your Changes

Separate independent logical changes into separate commits (and thus separate
patch submissions). This allows each change to be considered independently and
on it's own merits.  It is easier to review a batch of independent changes rather
than one large patch.

### Write Good Commit Messages

Commit messages become the public record of your changes, as such it is important
that they be well-written. If your [commit message sucks] (http://stopwritingramblingcommitmessages.com/)
nobody should accept your pull request.  The basic format of a 'good' git commit
messages is:

* A single summary line.  This should be short -- ideally no more than 50 characters
or so (with a hard maximum of 80 characters), since it can be used as the e-mail
subject when submitting your patch and for generating patch file names by
'git format-patch'.

* A blank line

* A detailed description of your change.  Where possible, write in the present tense.
If your changes have not resulted from a previous discussion, you should also
include a brief rationale for your change.  Your description should be formatted
as plain text with each line wrapped at 72 characters.

* If you are asked to change something, use 'git commit --amend'.

See Tim Pope's [excellent blog post] (http://tbaggery.com/2008/04/19/a-note-about-git-commit-messages.html)
for a more detailed explanation of the rationale behind many of these guidelines.

### Give Yourself Credit

Add yourself to the AUTHORS file with your commit, nobody else is going to do it
for you.  Contributions must be made under your real name or a well known alias
associated with your GitHub account.  Only maintainers have commit access.

Maintainers may commit changes to non-source documentation directly to the project,
but should otherwise not commit their own contributions.

### Copyrights and Licenses
All AUTHORS share in collective ownership of the code, there is no Copyright
Assignment process.

ZeroMQ and it's C4 process strongly favor the GPLv3, however this library exists
mainly to provide a Boost-consistent interface to ZeroMQ and therefore should be
usable under under the terms of the accompanying Boost license.

### Creating Stable Releases

Make sure your contributions do not include code from projects with incompatible
licenses.. AzioMQ is licensed under the BOOSTv1.0 licenses.
If your code isn't compatible with this, it will be spotted and removed.

### Code style

In general follow the existing style, specifically:
* Unix line endings
* Four (4) space indents
* Use spaces instead of tabs
* Soft 80 column limit (don't slop over too much)
* 'Line Saver' brace style
* JavaDoc style comments compatible with DOxygen
* 'Modern' C++11 style (yeah, vague, got it, just follow what is in the code
already)
* Use #include guards, not #pragma once
* Follow Boost's naming conventions as much as possible
