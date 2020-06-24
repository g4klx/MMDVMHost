# DMRplus - Startup Options

## Introduction
This file is to give an overview over the Options-parameter in MMDVM.ini [DMR Network]-section.

## Example
You can pull some conection-info at startup to the DMRplus-Network to define the behavior of TS1 and TS2 in DMR-mode. 
An example of such a line would be following:

  Options=StartRef=4013;RelinkTime=15;UserLink=1;TS1_1=262;TS1_2=1;TS1_3=20;TS1_4=110;TS1_5=270;

If an option is set, it overwrites the setting preset at the master, if an option is empty, it unsets a predefined setting from
the master. If an option is not set, the default from the master would be taken over.

## What the parameters are about?

Here is a quick explaination about the options to be set:

  * StartRef: This is the default reflector in TS2, in example: Refl. 4013
  * RelinkTime: This is the time to fall back to the default-reflector if linked to another one and no local traffic is done, 
  not yet implemented, would come next
  * UserLink: This defines, if users are allowed to link to another reflector (other than defined as startreflector)
    * 1 = allow
    * 0 = disallow
  * TS1_1: This is the first of 5 talkgroups that could be set static, in example: TG262
  * TS1_2: This is the second of 5 talkgroups that could be set static, in example: TG1
  * TS1_3: This is the third of 5 talkgroups that could be set static, in example: TG20
  * TS1_4: This is the fourth of 5 talkgroups that could be set static, in example: TG110
  * TS1_5: This is the fifth of 5 talkgroups that could be set static, in example: TG270

---
Info created by DG9VH 2016-11-11
