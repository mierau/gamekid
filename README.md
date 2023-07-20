# Gamekid
A community built Game Boy emulator for [Playdate](https://play.date).  
The emulator backend is provided by the equally teeny [PeanutGB](https://github.com/deltabeard/Peanut-GB).  
This is still very much a work in progress. FPS is not yet where we want it to be but it'll get there in time. Join in, help us out! :)

## Install
1. [Download](https://github.com/mierau/gamekid/releases/) the latest version of Gamekid.
2. [Sideload](https://play.date/account/sideload/) the resulting pdx on your Playdate.

## Install Games
The Game Boy has a bustling homebrew community. There are various sources online, but [Itch](https://itch.io/games/tag-gameboy/tag-homebrew) or [Homebrew Hub](https://gbhh.avivace.com/games) seem to be good places to start.
1. Reboot your Playdate into Data Disk mode (Settings > System > Reboot to Data Disk).
2. Connect your Playdate to your computer.
3. Open/select the Playdate drive when it appears.
4. Copy your .gb files into the `Data/<gamekid folder>/games` folder.
5. Unmount the Playdate.
6. That's it. Run Gamekid. Enjoy!

## Usage
Once installed and games stashed safely on your Playdate, using Gamekid is fairly straightforward. Simply open Gamekid.
Your games should load on the following screen. Select a game to run. Not all games run flawlessly and some have serious
FPS issues to the point of unplayability, but I'm convinced we can fix these in time.

Start/Select: Move the crank to activate start/select buttons.  
Open the Playdate menu for scaling and interlacing options.

## Building
1. If you're building on Apple silicon (M1, M2, etc.), make sure you have Rosetta installed as the ARM toolchain is built for Intel processors. You can do this on the command line: `softwareupdate --install-rosetta`
2. Review Panic's [How to use the C API](https://sdk.play.date/2.0.1/Inside%20Playdate%20with%20C.html#_how_to_use_the_c_api) doc. 
3. Grab a copy of [Playdate SDK](https://play.date/dev/) for your system.
4. Run `make` within the Gamekid folder. OR! grab yourself a copy of [Nova](https://nova.app) from [Panic](https://panic.com) (makers of the Playdate).

## Contributing
Gamekid is pretty good, but it isn't perfect. But we can get it there with your help!  
Connect with me on Twitter [@dmierau](https://twitter.com/dmierau)—I'm pretty active there (for better or worse).  
Simply make pull requests and I'll look over changes and merge.

## PeanutGB
Gamekid sits atop the wonderfully compact [PeanutGB](https://github.com/deltabeard/Peanut-GB) emulator backend. This emulator is designed for efficiency over accuracy.

## TODO
- Sound support
- Visual affordance for crank start/select
- FPS optimizations

## License
Matching the [PeanutGB](https://github.com/deltabeard/Peanut-GB) license here, which is MIT.