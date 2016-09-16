#!/usr/bin/ksh

### Some shortcuts defns and aliases useful while testing MLM ####
dd=/flx/FMSmlm/current/data
c=/flx/data/FMSmlm
caf=/flx/data/FMSmlm/config.appl
cdf=/flx/FMSmlm/current/data/config.default
ld=/var/flx/logs/FMSmlm
le=$ld/Error.log
lu=$ld/Usage.log
lm=$ld/Map.log

alias p="ps -aef | grep mlm"
alias rg="grep FLXMLM $RB/FLXcluster"
alias lcore="ls -l /var/corefiles"

machname=`uname -n`

alias on="RCCOnlineVM -m $machname FLXMLM"
alias off="RCCOfflineVM -m $machname FLXMLM"

alias tle="tail -f $le"
alias trl="tail -f $RL/rccout.log"

