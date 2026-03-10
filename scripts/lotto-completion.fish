# Dynamic Fish completion for lotto.
#
# This completion queries commands and flags at runtime:
#   - commands: `lotto --list-commands`
#   - flags:    `lotto [<cmd>] --list-flags`
# and caches results briefly for responsiveness.

function __fish_lotto_cache_key
    string replace -ar '[^A-Za-z0-9_.-]' '_' -- $argv[1]
end

function __fish_lotto_bin_mtime
    set -l bin $argv[1]
    if test -x "$bin"
        command stat -c %Y "$bin" 2>/dev/null
        return
    end
    if command -sq $bin
        set -l resolved (command -s $bin)
        command stat -c %Y "$resolved" 2>/dev/null
        return
    end
    echo 0
end

function __fish_lotto_cache_get
    set -l cache_id $argv[1]
    set -l bin $argv[2]
    set -e argv[1..2]

    set -l ttl 3
    if set -q LOTTO_COMPLETION_CACHE_TTL
        set ttl $LOTTO_COMPLETION_CACHE_TTL
    end

    set -l root $HOME/.cache/lotto-completion
    if set -q XDG_CACHE_HOME
        set root $XDG_CACHE_HOME/lotto-completion
    end
    if not command mkdir -p $root 2>/dev/null
        set root /tmp/lotto-completion
        command mkdir -p $root 2>/dev/null; or return
    end

    set -l file "$root/"(__fish_lotto_cache_key "$cache_id")
    set -l mtime (__fish_lotto_bin_mtime $bin)
    set -l now (date +%s)

    if test -r $file
        set -l hdr (command head -n 2 $file)
        set -l cached_mtime $hdr[1]
        set -l cached_ts $hdr[2]
        if test "$cached_mtime" = "$mtime"
            set -l age (math "$now - $cached_ts" 2>/dev/null)
            if test -n "$age"; and test $age -lt $ttl
                command tail -n +3 $file
                return
            end
        end
    end

    set -l out (command $argv 2>/dev/null)
    begin
        echo $mtime
        echo $now
        printf "%s\n" $out
    end >$file
    printf "%s\n" $out
end

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
    __fish_lotto_cache_get "$bin|commands" $bin $bin --list-commands
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
        __fish_lotto_cache_get "$bin|flags|$subcmd" $bin $bin $subcmd --list-flags
    else
        __fish_lotto_cache_get "$bin|flags|root" $bin $bin --list-flags
    end
end

for cmd in lotto ./lotto build/lotto ./build/lotto
    complete -c $cmd -f -n 'not __fish_lotto_has_subcmd' -a '(__fish_lotto_commands_raw)'
    complete -c $cmd -f -n 'not __fish_lotto_has_subcmd' -a '(__fish_lotto_flags_raw)'
    complete -c $cmd -f -n '__fish_lotto_has_subcmd' -a '(__fish_lotto_flags_raw)'
end
