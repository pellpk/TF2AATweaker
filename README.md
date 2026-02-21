## Serversided mod that allows servers to modify the strength of controller aim assist live.
**Requires client install.**

This was a bounty mod, not created by me, but the original creator wanted to stay anonymous.
Uses some code from [Northstar](https://northstar.tf/)

### recommended values

for reference base aa in the mod is "1", this is 0.4 tolerance ingame.

for competitive, i feel 0.4 (0.16) or 0.45 (0.18) is the balanced option

for more casual, i feel 0.5 (0.2) or a little higher is more balanced for less-experienced players

do your own testing, have fun

### cvars:

sv_soup_aimassist_multiplier = Global multiplier for controller aim assist strength.
Default 1. Set to numbers higher than 1 for funny interactions.

sv_soup_aimassist_snapshot_fix = Suppress snapshot snap-term for fire-before-ADS transitions. May cause issues.
boolean, default 0

sv_soup_aimassist_fps_independence_fix = Normalize unscaled aim-assist additive terms by frame time.
boolean, default  0

sv_soup_require_client_mod = Require clients to have the mod installed (kicks clients without the mod).
boolean, default 1

Original bounty video: https://www.youtube.com/watch?v=PfMJsnA4IAo

ModWorkShop link: https://modworkshop.net/mod/55448
