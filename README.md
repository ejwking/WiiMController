# WiiMController

WiiMController is a simple windows desktop app that lets you import a list of internet radio URLs, display them in the app, and launch the stream on your WiiM device.
The WiiM Android/iOS app has an equivalent of this functionality, its called the 'Open Network Stream'. However, the official WiiM windows desktop app has limited features, and doesn't include 'Open Network Stream', hence the reason for creating this app.

## Features
- Keep your list of internet radio stations in a .txt file, import this list into the app. Example text file is in examples folder.
- Select a stream in the UI to play on the WiiM device.
- Equaliser preset selection and basic player controls.
- View basic player info and metadata.

## Prerequisites
- Visual Studio (Desktop C++ workload)
- vcpkg
- cURL (libcurl), installed via vcpkg

To install vcpkg and libcurl follow instructions on the cURL website - https://curl.se/docs/install.html

## Build
1. Open the solution (`WiiMController.sln`) in Visual Studio.
2. Build the solution (F5).

## Limitations
- I wanted to keep this app simple, and the codebase tiny, so anyone can take a quick look and understand how it works and what its doing.
- I do not plan on adding device discovery or metadata polling, I think its unnecessary for this app.
- You must manually enter the IP of your device (which can be found in the WiiM Android/iOS app settings page).

## Screenshots

![screenshot](examples/screenshot.png)

## Third-Party Libraries
- nlohmann/json  
  https://github.com/nlohmann/json  
  Licensed under the MIT License.
- libcurl  
  https://curl.se/libcurl/  
  Licensed under the curl license.

## WiiMController License
- MIT license.
