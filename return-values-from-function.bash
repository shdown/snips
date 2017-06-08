«() {
    local -i _i=1
    while [[ ${!_i} != » ]]; do
        declare -n _${_i}=${!_i}
        (( ++_i ))
    done
    "${@:$[_i+1]}"
}

#-------------------------------------------------------------------------------

# Example 1:

get_message() {
    _1=Hello
}

« x » get_message
echo "$x"

# Example 2:

get_div_mod() {
    _1=$(( $1 / $2 ))
    _2=$(( $1 % $2 ))
}

« div mod » get_div_mod 5 2
echo "5 = 2 * $div + $mod"

# Example 3 (lexical scope):

h() {
    local x=$1
    _1=$x
}

g() {
    local x
    « x » h "$1"
    _1=$(( x + 1 ))
}

f() {
    local x
    « x » g "$1"
    « x » g "$x"
    _1=$(( x ** 2 ))
}

« x » f 5
echo "$x"
