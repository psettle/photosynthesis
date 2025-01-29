rm ../../../out/mcts.cpp
python3 ../../../modules/bundler/src/bundler.py -i "./main_mcts.cpp" -o "../../../out/mcts.cpp" -p "../../engine" -p "../../util"
g++ -O2 -Wall -Wextra ../../../out/mcts.cpp -o ../../../out/mcts.exe