# Dynamic Bash completion for lotto.
#
# This script discovers commands and flags at runtime:
#   - commands: `lotto commands`
#   - flags:    `lotto [<cmd>] -F`

_lotto__commands()
{
    local bin="$1"
    "$bin" --list-commands 2>/dev/null | cut -f1
}

_lotto__flags()
{
    local bin="$1"
    shift
    "$bin" "$@" --list-flags 2>/dev/null | cut -f1
}

_lotto__selected_subcmd()
{
    local bin="$1"
    local i token
    local commands
    commands="$(_lotto__commands "$bin")"

    for ((i = 1; i < cword; i++)); do
        token="${words[i]}"
        [[ "$token" == -* ]] && continue
        if grep -Fxq -- "$token" <<<"$commands"; then
            printf '%s' "$token"
            return 0
        fi
    done
    return 1
}

_lotto()
{
    local cur prev words cword
    _init_completion || return

    local bin="$1"
    local subcmd
    subcmd="$(_lotto__selected_subcmd "$bin")"

    case "$prev" in
        -i|--input-trace|-o|--output-trace)
            _filedir trace
            return
            ;;
        --filtering-config)
            _filedir conf
            return
            ;;
        --temporary-directory)
            _filedir -d
            return
            ;;
    esac

    if [[ -z "$subcmd" ]]; then
        COMPREPLY=( $(compgen -W "$(_lotto__flags "$bin") $(_lotto__commands "$bin")" -- "$cur") )
        return
    fi

    COMPREPLY=( $(compgen -W "$(_lotto__flags "$bin" "$subcmd")" -- "$cur") )
    if [[ ${#COMPREPLY[@]} -eq 0 && "$cur" != -* ]]; then
        _filedir
    fi
}

complete -F _lotto lotto ./lotto build/lotto
