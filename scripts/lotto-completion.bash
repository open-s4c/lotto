# Dynamic Bash completion for lotto.
#
# This script discovers commands and flags at runtime:
#   - commands: `lotto --list-commands`
#   - flags:    `lotto [<cmd>] --list-flags`
# and caches results briefly for responsiveness.

_lotto__cache_key()
{
    printf '%s' "$1" | tr '/ :\t' '____'
}

_lotto__bin_mtime()
{
    local bin="$1"
    local resolved=""
    if [[ -x "$bin" || -f "$bin" ]]; then
        resolved="$bin"
    else
        resolved="$(command -v "$bin" 2>/dev/null)" || return 0
    fi
    stat -c %Y "$resolved" 2>/dev/null || echo 0
}

_lotto__cache_get()
{
    local cache_id="$1"
    local bin="$2"
    shift 2

    local ttl="${LOTTO_COMPLETION_CACHE_TTL:-3}"
    local root="${XDG_CACHE_HOME:-$HOME/.cache}/lotto-completion"
    local key file now mtime
    key="$(_lotto__cache_key "$cache_id")"
    file="$root/$key"
    now="$(date +%s)"
    mtime="$(_lotto__bin_mtime "$bin")"

    if ! mkdir -p "$root" 2>/dev/null; then
        root="/tmp/lotto-completion"
        mkdir -p "$root" 2>/dev/null || return 0
        file="$root/$key"
    fi

    if [[ -r "$file" ]]; then
        local cached_mtime cached_ts
        cached_mtime="$(head -n 1 "$file")"
        cached_ts="$(head -n 2 "$file" | tail -n 1)"
        if [[ "$cached_mtime" == "$mtime" && "$cached_ts" =~ ^[0-9]+$ ]] &&
            (( now - cached_ts < ttl )); then
            tail -n +3 "$file"
            return 0
        fi
    fi

    local out
    out="$("$@" 2>/dev/null)"
    {
        printf '%s\n' "$mtime"
        printf '%s\n' "$now"
        printf '%s\n' "$out"
    } >"$file"
    printf '%s\n' "$out"
}

_lotto__commands_raw()
{
    local bin="$1"
    _lotto__cache_get "${bin}|commands" "$bin" "$bin" --list-commands
}

_lotto__commands()
{
    local bin="$1"
    _lotto__commands_raw "$bin" | cut -f1
}

_lotto__flags_raw()
{
    local bin="$1"
    shift
    _lotto__cache_get "${bin}|flags|$*" "$bin" "$bin" "$@" --list-flags
}

_lotto__flags()
{
    local bin="$1"
    shift
    _lotto__flags_raw "$bin" "$@" | cut -f1
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
