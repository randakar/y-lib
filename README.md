# What is Ylib?

Ylib is a set of libraries that can be added to HP Loadrunner test scripts.

# How
Documentation can be found at: http://randakar.github.io/y-lib/
Source code can be found at: https://github.com/randakar/y-lib

# Why?
The goal of Ylib is to make functionality and tools available that support performance testing and improve both development speed and test quality.

Standard loadrunner libraries are very limited and provide only very basic support for the type of scripts that are needed when you scale up to larger and more complicated test scenarios. Ylib strives to complement and enhance these libraries whereever doing so makes sense. 

# Design goals / Vision
The basic vision of Ylib is to provide even novice C programmers with a function library that is easy to use and does not require knowledge of C strings, character pointers or memory management. 
In order to do so it relies heavily on Loadrunners' built in parameter functionality. Most functions will accept and process parameters whereever possible, and store their results in parameters as well. This means that for most simple use cases whoever writes the script will not need to think more about the underlying language than they absolutely have to.
Many functions however do provide for more advanced concepts, such as extensive support for click flows (in the form of profiles), transaction hooks, and realistic browser emulation (supporting a list of browsers with various weights attached, rather than simply allowing for only a single user agent string and associated set of settings). 

# License
Ylib is an open source project and licensed under the terms of the GNU GPL, version 2.
( Since ylib is used and distributed as a piece of source code violation of this license should be almost impossible. )

# Proven 
Ylib has been used for many years with good success in various large organisations with virtual user counts well into the tens of thousands. In it's current incarnation it provides the most advanced form of loadtesting possible using Loadrunner, and possible in several other tools as well..

