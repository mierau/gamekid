# Gamekid
A community built Game Boy emulator for [Playdate](https://play.date).  
The emulator backend is provided by the equaly teeny [PeanutGB](https://github.com/deltabeard/Peanut-GB).

## Install
1. Download the latest version of Gamekid.
2. [Sideload](https://play.date/account/sideload/) the resulting pdx on your Playdate.

## Install Games
1. Reboot your Playdate into Data Disk mode (Settings > System > Reboot to Data Disk).
2. Connect your Playdate to your computer.
3. Copy your .gb files into the `app.gamekid.data/games` folder.
4. Unmount the Playdate.
5. That's it. Run Gamekid. Enjoy!

## Usage
Once installed and games stashed safely on your Playdate, using Gamekid is fairly straightforward. Simply open Gamekid.
Your games should load on the following screen. Select a game to run. Not all games run flawlessly and some have serious
FPS issues to the point of unplayability, but I'm convinced we can fix these in time.

Start/Select: Move the crank to activate start/select buttons.  
Open the Playdate menu for scaling and interlacing options.

## Building
1. Grab a copy of Playdate dev tools.
2. Run `make` within the Gamekid folder. OR! grab yourself a copy of [Nova](https://nova.app) from [Panic](https://panic.com) (makers of the Playdate).

## Contributing
Gamekid is pretty good, but it isn't perfect. But we can get it there with your help!  
Connect with me on Twitter [@dmierau](https://twitter.com/dmierau)—I'm pretty active there (for better or worse).  
Simply make pull requests and I'll look over changes and merge.

## PeanutGB
Though I had originally built Gamekid with my own emulator, it felt unnecessary for the world to
maintain yet another Game Boy emulator. So, I selected PeanutGB for it being written in C already
and its portable implementation. That said, I would like any improvements made to that library
to be contributed back.

## TODO
- Sound support
- Visual affordance for crank start/select
- FPS optimizations
