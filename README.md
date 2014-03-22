Tekdaqc Firmware
===============

This repository contains the source code and its supporting library for the Tekdaqc firmware. We are actively working to improve the code which operates the Tekdaqc and will update this repository with our progress, as well as field bug reports and pull requests. Additionally, feature requests may be made in our issues page and we will happily discuss them with you. 

Join [Intelligent Automation, Computer Interface, & DAQ Community](https://plus.google.com/u/0/communities/109351353187504550254) on [![DAQ Community on Google Plus](https://ssl.gstatic.com/images/icons/gplus-16.png)](https://plus.google.com/u/0/communities/109351353187504550254) to stay up-to-date on the latest news.

_NOTE: Currently the bootloader used for updating the Tekdaqc is not included in this repository. We still have some work to do in documenting and cleanup of the code before it is ready to be published. Stay tuned for updates._

## Using the Tekdaqc Firmware 

### Dependencies
1. None

### Environment
Tenkiv, Inc. uses Atollic TrueStudio for our development, and as such the project files for this environment are included. Atollic uses the GNU compiler under the hood and attempts have been made to adhere to ISO C and MISRA-C:2004 standards. Exceptions to both exist and there is ongoing development to eliminate these exceptions, in particular the MISRA-C:2004 exceptions. With this in mind, with some effort it should be possible to set up your own build environment based on the GNU ARM compiler, though the details of this are beyond the scope of this repository. We freely welcome any how-to's people wish to contribute and share and will post them and their credit prominently within this repository and its wiki. Additionally, any issues found which decrease the compatibility of our code with any compiler, we are open to comment on and will either accept pull requests or do our best to increase the cross compiler support.

If you plan to debug or flash the Tekdaqc, you will need a JTAG probe compatible with whichever IDE you are using. A low cost solution is the [ST-Link v2](http://www.st.com/web/en/catalog/tools/FM146/CL1984/SC724/SS1677/PF251168), which is compatible with nearly all ARM IDE's, Windows, and with the help of [another GitHub project](https://github.com/texane/stlink), Linux. 

### Setup

1. Clone or download a copy of the Tekdaqc Firmware source code.
2. Import the Tekdaqc Firmware Library project into TrueStudio.
3. Import the Tekdaqc Firmware project into TrueStudio.
4. You should be ready to go at this point.

## More Information

### Tekdaqc Firmware Wiki
* Please see our [wiki](https://github.com/Tenkiv/Tekdaqc-Firmware/wiki) for more detailed documetnation and information about the source code and its design. 

### Tekdaqc Manual
* Download the [Tekdaqc Manual here](http://www.tenkiv.com/tekdaqc_manual_pdf_v3.pdf)

### Other Links
* [Tenkiv Webpage](http://www.tenkiv.com/)
* [Intelligent Automation, Computer Interface, & DAQ Community](https://plus.google.com/u/0/communities/109351353187504550254) on [![DAQ Community on Google Plus](https://ssl.gstatic.com/images/icons/gplus-16.png)](https://plus.google.com/u/0/communities/109351353187504550254)

## Contributing

Please see our [contribution guidelines](https://github.com/Tenkiv/Tekdaqc-Firmware/blob/master/CONTRIBUTING.md) if you have issues or code contributions.

### Contributors
#### Tenkiv, Inc.
* [Jared Woolston](https://github.com/jwoolston)

## License

    Copyright 2013 Tenkiv, Inc.
    
    Licensed under the Apache License, Version 2.0 (the "License");
    you may not use this file except in compliance with the License.
    You may obtain a copy of the License at
    
    http://www.apache.org/licenses/LICENSE-2.0
    
    Unless required by applicable law or agreed to in writing, software
    distributed under the License is distributed on an "AS IS" BASIS,
    WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
    See the License for the specific language governing permissions and
    limitations under the License.
