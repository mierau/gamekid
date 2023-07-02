# This fork is not maintained, it was opened to add OS 2.0 compatibility to Gamekid 0.5.1. If a newer release of Gamekid is available, I recommend you use that instead.

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
3. Copy your .gb files into the `*.gamekid/games` folder.
4. Unmount the Playdate.
5. That's it. Run Gamekid. Enjoy!

## Usage
Once installed and games stashed safely on your Playdate, using Gamekid is fairly straightforward. Simply open Gamekid.
Your games should load on the following screen. Select a game to run. Not all games run flawlessly and some have serious
FPS issues to the point of unplayability, but I'm convinced we can fix these in time.

Start/Select: Move the crank to activate start/select buttons.  
Open the Playdate menu for scaling and interlacing options.

## Building
1. Grab a copy of [Playdate SDK](https://play.date/dev/) for your system.
2. Run `make` within the Gamekid folder. OR! grab yourself a copy of [Nova](https://nova.app) from [Panic](https://panic.com) (makers of the Playdate).

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
