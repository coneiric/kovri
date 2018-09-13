[<img width="300" src="https://static.getmonero.org/images/kovri/logo.png" alt="ˈKoʊvriː" />](https://gitlab.com/kovri-project/kovri)

1. [To cover, veil, wrap](https://en.wikipedia.org/wiki/Esperanto)
2. A free, decentralized, anonymity technology based on [I2P](https://getmonero.org/resources/moneropedia/i2p.html)'s open specifications

## Disclaimer
- Currently **Alpha** software; under heavy development (and not yet integrated with monero)

## Quickstart

- Want pre-built binaries? [Download below](#downloads)
- Want to build and install yourself? [Build instructions below](#building)

## Multilingual README
This page is also available in the following languages

- [Italiano](https://gitlab.com/kovri-project/kovri-docs/blob/master/i18n/it/README.md)
- Español
- Pусский
- [Français](https://gitlab.com/kovri-project/kovri-docs/blob/master/i18n/fr/README.md)
- [Deutsch](https://gitlab.com/kovri-project/kovri-docs/blob/master/i18n/de/README.md)
- Dansk

## Downloads

### Releases

Soon[tm]

### [Nightly Releases (bleeding edge)]()

Soon[tm]

## Coverage

| Type      | Status |
|-----------|--------|
| Coverity  | [![Coverity Status](https://scan.coverity.com/projects/7621/badge.svg)](https://scan.coverity.com/projects/7621/)
| Codecov   | [![Codecov](https://codecov.io/gl/kovri-project/kovri/branch/master/graph/badge.svg)](https://codecov.io/gl/kovri-project/kovri)
| License   | [![License](https://img.shields.io/badge/license-BSD3-blue.svg)](https://opensource.org/licenses/BSD-3-Clause)

## Building

### Dependencies and environment

| Dependency          | Minimum version              | Optional | Arch Linux  | Ubuntu/Debian    | macOS (Homebrew) | FreeBSD       | OpenBSD     |
| ------------------- | ---------------------------- |:--------:| ----------- | ---------------- | ---------------- | ------------- | ----------- |
| git                 | 1.9.1                        |          | git         | git              | git              | git           | git         |
| gcc                 | 4.9.2                        |          | gcc         | gcc              |                  |               |             |
| clang               | 3.5 (3.6 on FreeBSD)         |          | clang       | clang            | clang (Apple)    | clang36       | llvm        |
| CMake               | 3.5.1                        |          | cmake       | cmake            | cmake            | cmake         | cmake       |
| gmake (BSD)         | 4.2.1                        |          |             |                  |                  | gmake         | gmake       |
| Boost               | 1.58                         |          | boost       | libboost-all-dev | boost            | boost-libs    | boost       |
| OpenSSL             | Always latest stable version |          | openssl     | libssl-dev       | openssl          | openssl       | openssl     |
| Doxygen             | 1.8.6                        |    X     | doxygen     | doxygen          | doxygen          | doxygen       | doxygen     |
| Graphviz            | 2.36                         |    X     | graphviz    | graphviz         | graphviz         | graphviz      | graphviz    |
| Docker              | Always latest stable version |    X     | See website | See website      | See website      | See website   | See website |

#### Windows (MSYS2/MinGW-64)
* Download the [MSYS2 installer](http://msys2.github.io/), 64-bit or 32-bit as needed
* Use the shortcut associated with your architecture to launch the MSYS2 environment. On 64-bit systems that would be the `MinGW-w64 Win64 Shell` shortcut. Note: if you are running 64-bit Windows, you'll have both 64-bit and 32-bit environments
* Update the packages in your MSYS2 install:

```shell
$ pacman -Sy
$ pacman -Su --ignoregroup base
$ pacman -Syu
```

#### Install packages

Note: For i686 builds, replace `mingw-w64-x86_64` with `mingw-w64-i686`

`$ pacman -S make mingw-w64-x86_64-cmake mingw-w64-x86_64-gcc mingw-w64-x86_64-boost mingw-w64-x86_64-openssl`

Optional:

`$ pacman -S mingw-w64-x86_64-doxygen mingw-w64-x86_64-graphviz`

### Make and install

**Do *not* use the zip file from gitlab: do a recursive clone only**

```bash
$ git clone --recursive https://gitlab.com/kovri-project/kovri
$ cd kovri && make release  # see the Makefile for all build options
$ make install
```

- End-users MUST run `make install` for new installations
- Developers SHOULD run `make install` after a fresh build

### Docker

Or build locally with Docker

```bash
$ docker build -t kovri:latest .
```

## Documentation and Development
- A [User Guide](https://gitlab.com/kovri-project/kovri-docs/blob/master/i18n/en/user_guide.md) is available for all users
- A [Developer Guide](https://gitlab.com/kovri-project/kovri-docs/blob/master/i18n/en/developer_guide.md) is available for developers (please read before opening a pull request)
- More documentation can be found in your language of choice within the [kovri-docs](https://gitlab.com/kovri-project/kovri-docs/) repository
- [Moneropedia](https://getmonero.org/resources/moneropedia/kovri.html) is recommended for all users and developers
- [Forum Funding System](https://forum.getmonero.org/8/funding-required) to get funded for your work, [submit a proposal](https://forum.getmonero.org/7/open-tasks/2379/forum-funding-system-ffs-sticky)
- [repo.getmonero.org](https://repo.getmonero.org/monero-project/kovri) or monero-repo.i2p are alternatives to GitLab for non-push repository access
- See also [kovri-site](https://gitlab.com/kovri-project/kovri-site) and [monero/kovri meta](https://github.com/monero-project/meta)

## Vulnerability Response
- Our [Vulnerability Response Process](https://github.com/monero-project/meta/blob/master/VULNERABILITY_RESPONSE_PROCESS.md) encourages responsible disclosure
- We are also available via [HackerOne](https://hackerone.com/monero)

## Contact and Support
- IRC: [Freenode](https://webchat.freenode.net/) | Irc2P with Kovri
  - `#kovri` | Community & Support Channel
  - `#kovri-dev` | Development Channel
- Twitter: [@getkovri](https://twitter.com/getkovri)
- Email:
  - anonimal [at] getmonero.org
  - PGP Key fingerprint: 1218 6272 CD48 E253 9E2D  D29B 66A7 6ECF 9144 09F1
- [Kovri Slack](https://kovri-project.slack.com/) (invitation needed via Email or IRC)
- [Monero Mattermost](https://mattermost.getmonero.org/)
- [Monero StackExchange](https://monero.stackexchange.com/)
- [Reddit /r/Kovri](https://www.reddit.com/r/Kovri/)

## Donations
- Visit our [Donations Page](https://getmonero.org/getting-started/donate/) to help Kovri with your donations
