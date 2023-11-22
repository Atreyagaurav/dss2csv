_dss2csv() {
    local i cur prev opts cmd
    COMPREPLY=()
    cur="${COMP_WORDS[COMP_CWORD]}"
    prev="${COMP_WORDS[COMP_CWORD-1]}"

    
    opts="l t g h list timeseries grid help"
    if [[ ${COMP_CWORD} -eq 1 ]] ; then
	COMPREPLY=( $(compgen -W "${opts}" -- "${cur}") )
	return 0;
    fi
    
    COMPREPLY=( $(compgen -f -X '!*.dss' -- "${cur}") )
    return 0;
}

complete -F _dss2csv -o bashdefault -o dirnames dss2csv
complete -F _dss2csv -o bashdefault -o dirnames csv2dss
