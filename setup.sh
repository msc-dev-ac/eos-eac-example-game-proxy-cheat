PATH_TO_CLIENT="$HOME/Documents/eac/example_game_eos/client/dist/"
PATH_TO_SERVER="$HOME/Documents/eac/example_game_eos/server/dist/"
DIR=$(realpath "$(dirname "${BASH_SOURCE[0]}")")

start_server() {
    cd "$PATH_TO_SERVER"
    ./start_protected_game
    cd -
}

start_bad_client() {
    echo "$DIR/proxy.so" | sudo tee /etc/ld.so.preload
    cd "$PATH_TO_CLIENT"
    ./MyEOSGame 127.0.0.1 1234 -EOS_FORCE_ANTICHEATCLIENTNULL&
    cd -
    sleep 1
    sudo bash -c "echo '' > /etc/ld.so.preload"
    fg
}

start_good_client() {
    cd "$PATH_TO_CLIENT"
    ./start_protected_game 127.0.0.1 4321
    cd -
}

build() {
    gcc main.c -shared -o proxy.so -fPIC
}