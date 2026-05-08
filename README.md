# downhole-telemetry-simulation
Firmware for a fake oil-well sensor. It reads temperature and pressure from a sensor chip, packages the readings into compact data packets, and streams them out over a serial connection — kind of like how a real downhole tool sends measurements up to the surface while drilling. Uses a tiny operating system designed for microcontrollers.

## What is this Project?
This is similar to a capstone project/portfolio project to show proof of compentacy for SLB's digital technolgy internship position in the embedded systems department. We are creating a downhole-drilling telemetry device. "Tele" meaning distant and "metry" meaning measurement.

This is a device that sends signals to the user if something is going wrong.

## What are possible things that can go wrong in drilling?

Piezoelectric sensor to model 3-axis accelerometer. 