rm ../../../out/mcts.cpp
python3 ../../../modules/bundler/src/bundler.py -i "./main_mcts.cpp" -i "../../engine/engine_GameState.cpp" -o "../../../out/mcts.cpp" -p "../../engine" -p "../../util"
g++ -O2 -Wall -Wextra ../../../out/neural_mcts.cpp -o ../../../out/neural_mcts.exe