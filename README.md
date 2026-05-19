# WiiMController

WiiMController is a simple windows desktop app that lets you import a list of stream URLs and select one to play on your WiiM device.
This is basically an 'Open Stream Network' controller in a windows app.
I created this app because the official windows 'WiiM Home' app doesn't have the 'Open Stream Network' feature, and therefore isn't much use for controlling internet radio.

## Features
- Create your own .txt file containing a list of stream 'Name and URL', and import this list into the app.
- Select a stream in the UI to play on the WiiM device.
- View basic player and metadata.
- Load and select equaliser presets.

## Prerequisites
- Visual Studio (Desktop C++ workload)
- vcpkg (recommended)
- libcurl (installed via vcpkg)

To install vcpkg and libcurl follow instructions on the curl website - https://curl.se/docs/install.html

## Build
1. Open the solution (`WiiMController.sln`) in Visual Studio.
2. Build the solution (F5).

## Usage
- Prepare a .txt file, or use the example 'radio urls.txt' provided in this repo.
- In the app: use the "browse" and "load/refresh" buttons to import the file.
- This app does not (yet) do device discovery, in the UI you specify the IP address of your WiiM device (see screenshot). Your WiiM IP can easily be found in the WiiM phone app - click on Devices button at the bottom, then click on the cog wheel to take you to settings, then click on 'Network Status'.
- Click on a stream in the list control to play it on the WiiM device.

## License
- MIT license.

## Screenshots
