_dss2csv() {
    local i cur prev opts cmd
    COMPREPLY=()
    cur="${COMP_WORDS[COMP_CWORD]}"
    prev="${COMP_WORDS[COMP_CWORD-1]}"

    
    opts="l e"
    if [[ ${COMP_CWORD} -eq 1 ]] ; then
	COMPREPLY=( $(compgen -W "${opts}" -- "${cur}") )
	return 0;
    fi
    
    COMPREPLY=( $(compgen -f -X '!*.dss' -- "${cur}") )
    return 0;
}

complete -F _dss2csv -o bashdefault -o default dss2csv
