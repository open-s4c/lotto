# Bash completion support for Lotto
#
#
# To enable completion:
#   1) Copy this file to somewhere (e.g. ~/.lotto-completion.bash)
#   2) Add the following line to your .bashrc:
#       source ~/.lotto-completion.bash

_lotto()
{
    local cur prev words cword
    _init_completion || return

    if [[ $cur == -* && "$prev" == "$1" ]]; then
        COMPREPLY=( $( compgen -W "--help --version" -- "$cur" ) )
        return
    fi

    if [[ "$prev" == "$1" ]]; then
        COMPREPLY=( $( compgen -W "$( $1 --list-commands )" -- "$cur" ) )
        LOTTO_CHOSEN_CMD=$COMPREPLY
        return
    fi

    case $prev in
        -h|--help|--version)
            return
            ;;
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
        -s|--strategy)
            COMPREPLY=( $( compgen -W "pct pos random" -- "$cur" ) )
            return
            ;;
        --reconstruct-strategy)
            COMPREPLY=( $( compgen -W "random random_recursive" -- "$cur" ) )
            return
            ;;
    esac

    if [[ $cur == -* && "${LOTTO_CHOSEN_CMD}" != "" ]]; then
        COMPREPLY=( $( compgen -W '$( _parse_help "$1" "${LOTTO_CHOSEN_CMD} --help" )' -- "$cur" ) )
        return
    fi

    $1 --list-commands run | grep -wq "${LOTTO_CHOSEN_CMD}"
    if [[ $? -eq 0 ]]; then
        _filedir
        return
    fi

} &&
complete -F _lotto lotto ./lotto
