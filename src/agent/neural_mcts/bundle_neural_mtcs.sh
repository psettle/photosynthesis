rm ../../../out/neural_mcts.cpp
python3 ../../../modules/bundler/src/bundler.py -i "./main_neural_mcts.cpp" -i "../../engine/engine_GameState.cpp" -o "../../../out/neural_mcts.cpp" -p "../../engine" -p "../../util" -p "../../neural" -p "../../agent/mcts"
g++ -g -O0 -Wall -Wextra ../../../out/neural_mcts.cpp -o ../../../out/neural_mcts.exe