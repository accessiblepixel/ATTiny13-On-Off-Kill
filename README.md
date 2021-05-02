# ATTiny13-On-Off-Kill

This is a port of the ATTiny85 On/Off/Kill sketch for the ATTiny13A, originally written by Ralph Bacon ( https://github.com/RalphBacon/173-ATTiny85-Push-Button-On-Off-control )

It needs the TinyCore from MCUDude (I think that's the one. I'll check and hopefully remember to update later!).

Connections are shown in the sketch, and I have finally added a schematic I put together last year but subsequently lost and found again today.
PB2 is the only connection not shown in the schematic, which goes to the second microcontroller as the shutdown signal.

I hope this is useful to people, I did this as a challenge that Ralph gave me for working with the 13A, he said he would update one of his more complex projects to work within the contraints of the 13A's memory requirements and functionality and challenged me to do the same with his 85 on/off/kill circuit, and we both were able to achieve the goal of making each of our projects work on the 13A.

The project started out as a bit of a query I had for measuring lipo battery current and sort of moved on from there, with ralph creating this awesome sketch so that we didn't have to use a fairly pricy LTC chip that wasn't as customisable as this and was more expensive.

Thanks for the help, inspiration and guidance Ralph! :)
