## Serversided mod that allows servers to modify the strength of controller aim assist live.
**Requires client install.**

This was a bounty mod, not created by me, but the original creator wanted to stay anonymous.
Uses some code from [Northstar](https://northstar.tf/)

### cvars:

sv_soup_aimassist_multiplier = Global multiplier for controller aim assist strength.
can be any value between 1 and 0. Default 1

sv_soup_aimassist_snapshot_fix = Suppress snapshot snap-term for fire-before-ADS transitions. May cause issues.
boolean, default 0

sv_soup_aimassist_fps_independence_fix = Normalize unscaled aim-assist additive terms by frame time.
boolean, default  0

sv_soup_require_client_mod = Require clients to have the mod installed (kicks clients without the mod).
boolean, default 1

Original bounty video: https://www.youtube.com/watch?v=PfMJsnA4IAo
