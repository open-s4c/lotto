# Dynamic Fish completion for lotto.
#
# This completion queries commands and flags at runtime:
#   - commands: `lotto --list-commands`
#   - flags:    `lotto [<cmd>] --list-flags`

function __fish_lotto_bin
    set -l tokens (commandline -opc)
    if test (count $tokens) -ge 1
        set -l bin $tokens[1]
        if test -x "$bin"
            echo $bin
            return 0
        end
    end
    if command -sq lotto
        echo lotto
        return 0
    end
    if test -x ./build/lotto
        echo ./build/lotto
        return 0
    end
    return 1
end

function __fish_lotto_commands_raw
    set -l bin (__fish_lotto_bin); or return
    command $bin --list-commands 2>/dev/null
end

function __fish_lotto_commands
    __fish_lotto_commands_raw | cut -f1
end

function __fish_lotto_selected_subcmd
    set -l tokens (commandline -opc)
    set -e tokens[1]
    set -l commands (__fish_lotto_commands)
    for t in $tokens
        if string match -q -- '-*' $t
            continue
        end
        if contains -- $t $commands
            echo $t
            return 0
        end
    end
    return 1
end

function __fish_lotto_has_subcmd
    __fish_lotto_selected_subcmd >/dev/null
end

function __fish_lotto_flags_raw
    set -l bin (__fish_lotto_bin); or return
    set -l subcmd (__fish_lotto_selected_subcmd)
    if test -n "$subcmd"
        command $bin $subcmd --list-flags 2>/dev/null
    else
        command $bin --list-flags 2>/dev/null
    end
end

for cmd in lotto ./lotto build/lotto ./build/lotto
    complete -c $cmd -f -n 'not __fish_lotto_has_subcmd' -a '(__fish_lotto_commands_raw)'
    complete -c $cmd -f -n 'not __fish_lotto_has_subcmd' -a '(__fish_lotto_flags_raw)'
    complete -c $cmd -f -n '__fish_lotto_has_subcmd' -a '(__fish_lotto_flags_raw)'
end
