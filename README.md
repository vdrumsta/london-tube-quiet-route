# London Tube Quiet Route Finder

<!-- ABOUT THE PROJECT -->
## About The Project

London Tube Quiet Route Finder is an itinerary recommendation engine that generates London Tube trips based on real-time network load. The main aim is to find routes that avoid crowded stations but it can also find the fastest route if you don't care about how crowded the stations are. Quiet routes are noticeably less busy, but they may be slightly longer.

![london-tube](https://github.com/worm00111/london-tube-quiet-route/blob/master/images/london-underground-station.jpg?raw=true)

The project is mainly based on [Learn C++ Through Projects](https://www.learncppthroughprojects.com/) tutorial series which teaches how to write production quality code using strong TDD practices.

## Architecture and Design

The project is made up of 2 main parts:
*A websocket client which listens to when passengers enter and leave a station
*A websocket server which serves a quiet route recommendation

Both of these websocket parts communicate using the [STOMP](https://stomp.github.io/) protocol.

When the program first launches, it fetches the network map of stations, lines and routes and builds an internal representation of it.
The websocket client receives the passenger enter/leave events and lets the station network track of how busy each station is.

When the quiet route recommendation server receives a request, it uses Dijkstra's algorithm to find a fastest route between station A and station B and then uses Yen's algorithm to find a suitable alternative route which is reasonably quieter than the fastest route. It then encodes the response in a STOMP frame and sends it back to the requester.

![architecture](https://github.com/worm00111/london-tube-quiet-route/blob/master/images/architecture.png?raw=true)

### Built With

* [C++17](https://en.wikipedia.org/wiki/C%2B%2B)
* [CMake](https://ninja-build.org/)
* [Boost.Asio library](https://www.boost.org/doc/libs/1_76_0/doc/html/boost_asio.html)

<!-- GETTING STARTED -->
## Getting Started

To get a local copy up and running follow these simple steps.

### Prerequisites

* Install a preferred C++17 compiler. I use [GCC 9.3](https://www.gnu.org/software/gcc/gcc-9/)
* Build System - [ninja](https://github.com/ninja-build/ninja/) (v1.8 or newer)
* Build Generator - [CMake](https://cmake.org/install/) (v3.18 or newer)
* Dependency Manager - [conan](https://docs.conan.io/en/latest/installation.html)

### Installation

1. Clone the repo
   ```sh
   git clone https://github.com/worm00111/london-tube-quiet-route.git
   ```
2. Install NPM packages
   ```sh
   cd london-tube-quiet-route/build
   conan install .. --profile ../conanprofile.toml --build missing
   ```
3. Generate Build System files
   ```sh
   cmake .. -GNinja -DCMAKE_BUILD_TYPE=Release
   ```
4. Compile the generated cmake files into a C++ program
   ```sh
   ninja
   ```
5. Run the program. Must stand in tests folder because that's where the certificate file is in.
   ```sh
   cd ../tests
   ../build/network-monitor-exe
   ```

<!-- Limitations -->
## Limitations
* The recommendation engine does not take the direction of travel into account. In the morning rush hour, you'll often find that it's more crowded if you're travelling towards the city centre. Recommendation engine doesn't take direction into account and only looks at how crowded stations are.
* If a station is served by several lines, one line might be significantly less busier than another, but the recommendation engine doesn't take this into account. In general, it will try to avoid crowded stations, rather than crowded lines.


<!-- ROADMAP -->
## Roadmap

There are a couple of areas I'd like to improve on to make this program more scalable:
* Optimize the performance of the recommendation engine by benchmarking the slow parts
* Add a CI automated build environment


<!-- CONTACT -->
## Contact

Vilius Drumsta - vdrumsta@gmail.com

Project Link: [https://github.com/worm00111/london-tube-quiet-route](https://github.com/worm00111/london-tube-quiet-route)