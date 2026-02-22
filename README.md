# Welcome to the home of cDynAA and pyDynAA

This repository is the public gate for the discrete event simulation engine cDynAA and its python front-end, pyDynAA.

The development of this simulation engine started in 2013 within the Dutch company TNO.  It's development is now made public in 2026.

**cDynAA** is a high-performance, low memory footprint discrete event simulation (DES) engine.  It was started in 2013, as the base simulation engine for distributed sensor systems.
It quickly evolved to simulation of embedded computing systems, with capabilities to model and simulate small snippets of program running on many, spatially distributed, networked processors.
At this time, cDynAA was developed in Java due to its portability.  Soon, it became clear that the Java virtual machine was not efficient enough for large simulations.

In 2015, the development team was invited to work together with QTech (TUDelft, TNO) on a simulator for quantum networks -- NetSquid.
The DynAA team ported the engine to C++ and called it cDynAA.  It also created a python shell (interface layer) using Cython.  We call that shell **pyDynAA**.  
The resulting engine became extremely fast, able to handle large number of components with a low memory footprint.

Based on experience of the first years, the team kept the **core** layer (cDynAA/pyDynAA) separate from the domains specific libraries that target a certain domain.  
This way, it is possible to write simulators for different domains using the same principle and underlying engine.
Examples of simulators written with cDynAA are:

* [NetSquid](https://netsquid.org/) - The **Net**work **S**imulator for **Qu**antum **I**nformation using **D**iscrete events (NetSquid) is a software tool for the modelling and simulation of scalable quantum networks developed at QuTech. The goal of NetSquid is to enable scientists and engineers to design the future quantum internet as well as modular quantum computing architectures.
* [nexussim](https://github.com/TNO/nexussim) - NexusSim is a cutting-edge simulator designed to assess the performance of applications deployed across the cloud-fog-edge nexus. It empowers researchers and developers to model, analyze, and optimize interactions in distributed systems, uncovering insights into resource usage and application behavior across all layers of the computing continuum.

This first README is just a corner stone for the transfer of the project from the internal repositories to the external and public one.
