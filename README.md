# mender-mcu-client

[![CI Badge](https://github.com/joelguittet/mender-mcu-client/workflows/ci/badge.svg)](https://github.com/joelguittet/mender-mcu-client/actions)
[![Issues Badge](https://img.shields.io/github/issues/joelguittet/mender-mcu-client)](https://github.com/joelguittet/mender-mcu-client/issues)
[![License Badge](https://img.shields.io/github/license/joelguittet/mender-mcu-client)](https://github.com/joelguittet/mender-mcu-client/blob/master/LICENSE)

[![Bugs](https://sonarcloud.io/api/project_badges/measure?project=joelguittet_mender-mcu-client&metric=bugs)](https://sonarcloud.io/dashboard?id=joelguittet_mender-mcu-client)
[![Code Smells](https://sonarcloud.io/api/project_badges/measure?project=joelguittet_mender-mcu-client&metric=code_smells)](https://sonarcloud.io/dashboard?id=joelguittet_mender-mcu-client)
[![Duplicated Lines (%)](https://sonarcloud.io/api/project_badges/measure?project=joelguittet_mender-mcu-client&metric=duplicated_lines_density)](https://sonarcloud.io/dashboard?id=joelguittet_mender-mcu-client)
[![Lines of Code](https://sonarcloud.io/api/project_badges/measure?project=joelguittet_mender-mcu-client&metric=ncloc)](https://sonarcloud.io/dashboard?id=joelguittet_mender-mcu-client)
[![Vulnerabilities](https://sonarcloud.io/api/project_badges/measure?project=joelguittet_mender-mcu-client&metric=vulnerabilities)](https://sonarcloud.io/dashboard?id=joelguittet_mender-mcu-client)

[![Maintainability Rating](https://sonarcloud.io/api/project_badges/measure?project=joelguittet_mender-mcu-client&metric=sqale_rating)](https://sonarcloud.io/dashboard?id=joelguittet_mender-mcu-client)
[![Reliability Rating](https://sonarcloud.io/api/project_badges/measure?project=joelguittet_mender-mcu-client&metric=reliability_rating)](https://sonarcloud.io/dashboard?id=joelguittet_mender-mcu-client)
[![Security Rating](https://sonarcloud.io/api/project_badges/measure?project=joelguittet_mender-mcu-client&metric=security_rating)](https://sonarcloud.io/dashboard?id=joelguittet_mender-mcu-client)

Mender is an open source over-the-air (OTA) software updater for embedded devices. Mender comprises a client running at the embedded device, as well as a server that manages deployments across many devices.

This repository is not a fork of [mender](https://github.com/mendersoftware/mender) ! It has the same behavior but it is written to be integrated on MCU projects. As a consequence it's a very light version written in C designed to be cross platform to fit any existing projects wanted to offer over-the-air capabilities.

Robustness is ensured with *atomic* image-based deployments using a dual A/B partition layout on the MCU. This makes it always possible to roll back to a working state, even when losing power at any time during the update process.

The project currently support firmware upgrade which is the main interest, and inventory key-value pair management. It will also provide in a near future some other Mender features that could make sense on MCU.

![Mender logo](https://github.com/mendersoftware/mender/raw/master/mender_logo.png)


## Getting started

To start using Mender, we recommend that you begin with the Getting started section in [the Mender documentation](https://docs.mender.io/).

To start using mender-mcu-client, we recommend that you begin with one of the example provided with the library:

* [mender-esp32-example](https://github.com/joelguittet/mender-esp32-example) demonstrates the usage of mender-mcu-client on ESP32 MCU to perform upgrade with dual OTA partitions and rollback capability using ESP-IDF framework.
* [mender-stm32l4a6-zephyr-example](https://github.com/joelguittet/mender-stm32l4a6-zephyr-example) demonstrates the usage of mender-mcu-client on STM32L4A6 MCU to perform upgrade with dual OTA partitions and rollback capability using Zephyr RTOS.
* More examples will come, and particularly new platforms support will be added soon.


## Dependencies

The mender-mcu-client library tries to be very light in order to be used on most projects wanted to have over-the-air updates. The library mainly rely on the presence of an RTOS to have thread, semaphore and memory allocation support.

Additionally, a TCP/IP interface is required because communications are done using HTTPS protocol, however it is possible to have an abstraction for example using a WiFi module connected to the MCU with a serial interface and using AT commands.

And finally, 4kB of storage should be reserved to save client private and public keys used for authentication with mender server, plus OTA ID and artifact name to be deployed when an update is done (this is used internally to perform OTA report to the server).

From the source code perspective, the dependencies of the core part of the library are limited to [cJSON](https://github.com/DaveGamble/cJSON). The platform source files may depends of external libraries or Hardware Abstraction Layers: [esp-idf](https://github.com/espressif/esp-idf), [mbedTLS](https://github.com/Mbed-TLS/mbedtls/), ...


## Generating mender-artifact

The examples provide details to generate properly the artifact to be used. The most important is to create artifact with no compression at all, because the mender-mcu-client is not able to manage decompression of the artifact on the fly (it is flashed by blocks during the reception from the mender server).

To create a new artifact first retrieve [mender-artifact](https://docs.mender.io/downloads#mender-artifact) tool. Then you should follow the template below to create the artifact:

```
./mender-artifact write rootfs-image --compression none --device-type <DEVICE-TYPE> --artifact-name <ARTIFACT-NAME> --output-path <ARTIFACT-FILE>.mender --file <BIN-FILE>
```

Where:
* `DEVICE-TYPE` is the device type to be used on mender, for example `my-super-device`.
* `ARTIFACT-NAME` is the artifact name to be used on mender, it should include the version number, for example `my-artifact-v1.0.0`.
* `ARTIFACT-FILE` is the artifact output file to be uploaded on mender server.
* `BIN-FILE` is the application binary file to be flashed on the MCU in the next OTA partition.


## API

The API of the mender-mcu-client is defined in `include/mender-client.h` file and the source code is documented to use it properly. The easiest way to start is using an example that is illustrating the usage of the library.


## Contributing

This library is created just before I think Mender is cool and it was cool to do it on MCU and finally it's even better to share it and see it's growing.

This is why I welcome and ask for your contributions. You can for example:

* provide new examples based on existing ones.
* provide new platforms support (please read following Architecture section).
* enhance the current implementation of platform support submitting a Pull Request (please pay attention to the results of the analysis done by the CI, code quality is important).
* open an issue if you encountered a problem with the existing implementation, and optionally provide a pull request to fix it, this will be reviewed with care.
* open an issue to discuss a new feature you want (please read following Roadmap section to avoid claiming features that are already in the list, but of course you are free to open an issue to discuss of future features to ask for the progress or just inform about your interest).
* ...


## Architecture

The organization of the source code permits the mender-mcu-client library to provide support for different kind of platforms and projects.

The source code is separate into three main directories:
* `core` contains the main source files providing the implementation of the mender-mcu-client:
    * `mender-client`: periodically check the availability of updates.
    * `mender-api`: the implementation of the mender [Device APIs](https://docs.mender.io/api/#device-apis).
    * `mender-untar`: TAR parser to read [mender artifact](https://github.com/mendersoftware/mender-artifact/blob/master/Documentation/artifact-format-v3.md).
    * `mender-utils`: utilities.
* `platform` contains source files specific to the platform or project, it is separated in several sub-directories for each feature of the client that rely on external dependency specific to the platforms:
    * `mender-ota`: implementation of functions used to write artifact in the memory.
    * `mender-storage`: provide storage area for the mender-client.
    * `mender-log`: logging API.
    * `mender-http`: implementation of HTTP client.
    * `mender-rtos`: implementation of RTOS related functions.
    * `mender-tls`: provide TLS support.
* `add-ons` contains source files of the mender add-ons:
    * `mender-inventory`: provide inventory key/value pairs to display inventory data on the mender server. This add-on is highly recommended and should be included by default. It is proposed as an add-on only to give the possibility to reduce the final code size for user who don't need it.

The `include` directory contains the header files that define the prototypes for each module of the library. They are common to all platforms and projects and they define the API to be used or implemented.

The usage of the mender-mcu-client library should be to include all the `core` source files each time, and then to pick up wanted platform implementations, or to write your owns (but it's better to share them and open a Pull Request!). For example you may want to use esp-idf with mbedTLS or with a secure element, or using an other RTOS I have not integrated, or maybe you want to have mender storage located in an external EEPROM. The combinaisons are infinite.

The usage of the add-ons is at your own discretion, they are independant.

The final code size of the mender-mcu-client library depends of your configuration, the following table gives an estimation.

|                      | Code size (-Og) | Code size (-Os) |
|:---------------------|:----------------|:----------------|
| mender-client (core) | 22KB            | 20KB            |
| mender-inventory     | 2KB             | 2KB             |


## Roadmap

The following features are currently in the pipeline. Please note that I haven't set dates in this roadmap because I develop this in my free time, but you can contribute with Pull Requests:

* Support update of [modules](https://docs.mender.io/artifact-creation/create-a-custom-update-module) to perform other kind of updates that could be specific to one project: files, images, etc.
* Integration of other nice to have Mender APIs as new add-ons: [Device Configure](https://docs.mender.io/api/#device-api-device-configure), [Device Monitor](https://docs.mender.io/api/#devices-api-device-monitor), remote console...
* Support new board and prove it is cross-platform and that it is able to work on small MCU too: STM32F7, ATSAMD51...
* Integration of ATECC608B secure element to perform TLS authentication.
* Support other RTOS (particularly Azure RTOS) and bare metal platforms.
* ...


## License

MIT
