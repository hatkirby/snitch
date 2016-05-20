# snitch
A simple Twitter bot that can respond to a trigger. `snitch` scans incoming tweets for the phrase "calling the cops", to which it responds with the following image:

![Call the police.](https://raw.githubusercontent.com/hatkirby/snitch/master/image.jpg)

`snitch` only interacts with users it is following. Following `snitch` causes it to follow you back; because Twitter does not send unfollow messages on the user stream however, it cannot automatically unfollow someone when they unfollow it, and instead, it checks its list of followers every few hours.

The bot was inspired by Twitter users [@cymrin](https://twitter.com/cymrin) and [@KbLogQ](https://twitter.com/KbLogQ) and was implemented alongside user stream functionality in my [libtwitter++](https://github.com/hatkirby/libtwittercpp) C++ Twitter library.
