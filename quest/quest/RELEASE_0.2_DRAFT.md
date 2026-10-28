# MallVR Quest 0.2 + PC v0.12.0: online crossplay release (DRAFT, nothing uploaded or posted)

Bobby 10-26: "we will just release it out in the wild", "let the world test it", "we get so little traffic it has to be this way and
you gather reports". Same route as Quest 0.1 / PC v0.11.3.

## What goes up (each step only after Bobby's go on this list)
1. **list.php** (online/list.php) -> downloads.gainesphere.com/mallvr/online/list.php (backup the live one first). From then on
   v0.11.x copies see the list but can only join on the same home network; the update prompt brings them to v0.12.0.
2. **PC v0.12.0**: build.ps1 -Release (no test player) -> make_package.py v0.12.0 -> make_latest.py (signed pointer) -> upload the
   zip + .sig + pointer as for v0.11.3; Citizen copy + its pointer with sign_release.ps1.
3. **Quest 0.2**: H:\Android\release\0.2\ (APK, source zip, QUEST_INSTALL.txt) -> downloads.gainesphere.com/mallvr/quest/ next to 0.1;
   check SHA-1 after upload.
4. **Website** Quest card (/pages/mallvr-download#quest): 0.2 download, "NEW: online with PC players" line; crossplay "coming"
   text replaced by "online crossplay: TEST". Credits as the page has them (Jeremy Shartzer, ErrorDan, nyxxic + Faulty, Simon Marston).
5. **One Discord post** in #announcements, with a picture (the 14-game picture or the MallVR logo; no Quest screenshot exists).
   Facebook / X only if Bobby wants (FB = Anonymous participant, check the toggle).

## Discord draft
> **MallVR goes online across PC and Quest (test build)!** MallVR Quest 0.2 and PC v0.12.0 put PC and Quest players in the same
> online list: QUICK MATCH, HOST or JOIN, and groups stay together between games. Also new: VOICE CHAT on the Button Masher Menu
> (turn each friend up or down) and private host addresses. This is a test: if it works or breaks, press REPORT A PROBLEM and
> keep play notes on, that's how we fix it fast. PC players: update when MallVR asks. Quest: <link>.
> Thanks to Jeremy Shartzer, ErrorDan, nyxxic and Faulty, and Simon Marston!

## After release (reports)
- report.php (op=list) + the email alerts to gainesphere@: every report, Quest ones marked QUEST 0.2-quest.
- stats.php export: plat=QUEST game_end lines with how=online (pod, ended, fps), plus the PC side's game_end for the same match.
- Watch for: Quest never reaching the list (HTTPS / Wi-Fi), "NO ANSWER: HOST ROUTER MAY BE CLOSED" (NAT: relay server next),
  game process killed while online (the service), desync in a Quest + PC game, the Boxing remote seat, Buggy Ball 4 pods.

## Privacy page line (gainesphere.com/pages/mallvr-privacy), BEFORE the release goes live
Add under play notes: "Online play (only with play notes on): whether joining a game worked and why not, how you were connected
(home network or internet, whether your router opened a port), the lag between players, the game's network counts, whether your mic
was used (never what you said), how long you waited for a match and how many players you saw, which platform each player in your
group was on, and on a Meta Quest the Wi-Fi signal strength and band."

## New play-note records (10-26, PC + Quest): what to read after release
online_session (one ONLINE visit: seen_max, open_max, quick, hosted, joined, games, wait_secs, matched, ended), online_fail (why,
role, via, host_plat, stun, router, list, in_game), list_fail (why the list couldn't be reached), net (after each online game:
net_tx / net_rx / net_drop / foreign / voice_tx / voice_rx / mic), online (+ group with PC/QUEST, + wifi on Quest), group_switch.
