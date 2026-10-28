# Contributing

Fixes, ports, new tools and new games are welcome as pull requests.

- **Bugs:** open an [issue](https://github.com/gainesphere-studios/MallVR/issues). Include the game, your headset (or
  window mode), what happened, and the logs from MallVR's `logs\` folder (`<game>.render`, `<game>.calls`,
  `<game>.net`). REPORT A PROBLEM in the launcher makes the same zip.
- **Code:** keep changes small and focused. Match the style of the file you're in. Explain *why* in comments where
  the reason isn't obvious. Most of this code exists because of something a 30-year-old game does.
- **New games:** see [Adding a game](adding-a-game.md). Hook tables are welcome even if the game isn't fully working
  yet. Mark what's missing with `#` comments.
- **Game files:** never commit game EXEs, CD images, or any files from the games. MallVR ships no game files.
- **Knowledge:** if you worked on, ran or owned Virtuality pods, your stories, photos, manuals and settings help as much
  as code. Bring them to the [Discord](https://discord.gg/7tDn2AAXEC).
- **Licence:** contributions are under the GNU GPL v2 or later, like DOSBox-X.

## Credits

- [DOSBox-X](https://github.com/joncampbell123/dosbox-x) and its contributors: the PC emulator MallVR is built on
- [LeonG-ZA](https://github.com/LeonG-ZA/dosbox-x) (branch `su2000`): the low-level SU2000 card emulation in `su2000/`
- Valve: the OpenVR SDK header
