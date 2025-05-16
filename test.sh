#!/bin/bash

# テストファイルと一時ディレクトリの設定
TMP_FILE=".test_output"
TEST_DIR=".test_dir_microshell"
mkdir -p $TEST_DIR

# 色設定
GREEN="\033[0;32m"
RED="\033[0;31m"
RESET="\033[0m"

# コンパイルチェック
echo "🔨 Compiling..."
gcc try.c -o microshell
if [ $? -ne 0 ]; then
  echo -e "${RED}❌ Compilation failed.${RESET}"
  exit 1
fi
echo -e "${GREEN}✅ Compilation OK${RESET}"

# ヘルパー関数
run_test() {
  DESC="$1"
  shift
  CMD="$@"
  eval "$CMD" > $TMP_FILE 2>&1
  if [ $? -eq 0 ]; then
    echo -e "${GREEN}✅ $DESC${RESET}"
  else
    echo -e "${RED}❌ $DESC${RESET}"
    cat $TMP_FILE
  fi
}

# 1. セミコロンでコマンド連続実行
run_test "Command separator ';'" \
  './microshell /bin/echo first ";" /bin/echo second | diff - <(echo -e "first\nsecond")'

# 2. パイプ処理
run_test "Pipe command '|'" \
  './microshell /bin/echo hello "|" /usr/bin/tr a-z A-Z | diff - <(echo "HELLO")'

# 3. cd 成功
run_test "cd command success" \
  "./microshell cd $TEST_DIR ';' /bin/pwd | grep -q \"$(realpath $TEST_DIR)\""

# 4. cd 引数なし
run_test "cd with no arguments" \
  './microshell cd 2>&1 | grep -q "error: cd: bad arguments"'

# 5. cd 失敗
run_test "cd with invalid path" \
  './microshell cd /notexist 2>&1 | grep -q "error: cd: cannot change directory to /notexist"'

# 6. 実行不能なコマンド
run_test "execve failure" \
  './microshell ./does_not_exist 2>&1 | grep -q "error: cannot execute ./does_not_exist"'

# 7. 多段パイプ
PIPE_CMD=""
for i in {1..100}; do
  PIPE_CMD="$PIPE_CMD /bin/echo $i |"
done
PIPE_CMD="$PIPE_CMD /bin/cat"
run_test "100+ pipe test (no FD leak)" "./microshell $PIPE_CMD | tail -n 1 | grep -q 100"

# 8. valgrind leak check
if command -v valgrind &> /dev/null; then
  echo "🧠 Running valgrind..."
  valgrind --leak-check=full --error-exitcode=1 ./microshell /bin/echo test > /dev/null 2>&1
  if [ $? -eq 0 ]; then
    echo -e "${GREEN}✅ No memory leaks detected${RESET}"
  else
    echo -e "${RED}❌ Memory leaks detected (see valgrind output)${RESET}"
  fi
else
  echo "⚠️  Valgrind not installed. Skipping memory leak check."
fi

# 後始末
rm -rf $TEST_DIR $TMP_FILE microshell
