#Version structure: A.B.C.D:

 - __A: Major Version:__ Incremented for hardware changes, re-architectures, and major functionality differences.
 - __B: Minor Version:__ Incremented for additions of new features.
 - __C: Build Number:__ Incremented for bug fixes and minor improvements within the current feature set.
 - __D: Special Build:__ 0 unless the build is customized for some particular purpose such as hardware verification or experimental testing.

#Release history:

###Release: 1.0.0.0
 - __Date:__    3/19/2014
 - __Brief:__   Initial release.
 - __Notes:__   N/A
 - __Link:__   [v1.0.0.0](https://github.com/Tenkiv/Tekdaqc-Firmware/releases/tag/v1.0.0.0.0)

 ###Release: 1.0.1.0
 - __Date:__    4/16/2014
 - __Brief:__   Bug Fixes.
 - __Notes:__   
   - Fixes a bug in continuous digital input sampling, causing it to terminate unexpectedly.
  - Fixes a bug in the self calibration on board start up, causing some samples to read as 0.
  - Fixes a bug in ADD_DIGITAL_OUTPUT command parsing.
  - Fixes a bug in SET_DIGITAL_OUTPUT command parsing.
  - Improved performance of digital input multisampling.
  - Bug fixes and improvements for analog input calibration.
  - Fixes a bug in analog input multisampling.
  - Fixes a parsing bug in all READ_* commands when specifying a set or range.
 - __Link:__   [v1.0.0.0](https://github.com/Tenkiv/Tekdaqc-Firmware/releases/tag/v1.0.0.0.0)