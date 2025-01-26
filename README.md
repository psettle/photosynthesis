# photosynthesis
Photosynthesis Agent.

This is an implementation of a legend-level agent for the codingame Spring Challenge 2021, which is based on the photosynthesis board game.

It uses Monte Carlo Tree Search in a very normal/standard configuration.

The rollout and backup strategies are significantly based on this post:
https://forum.codingame.com/t/spring-challenge-2021-feedbacks-strategies/190849/4

Largely this implementation is an exercise in optimization.

It makes heavy use of bitwise operations, compiler optimizations, GCC built-ins, and constexpr lookup tables to make the game state representation small and to make move enumeration efficient.

The way shadows fall is pre-computed for every direction, size, and position.
The valid seed destinations are pre-computed for every size and position.

The board is represented with a series of bitfields. bitfields are efficiently iterated with built-in 'count leading zeros' operations.

This implementation can simulate around 30000 games/s on the current Codingame servers.

