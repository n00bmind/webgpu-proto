
function! P4Checkout()
    " Signal that the file is gonna be changed externally and this should be ignored
    let s:ignore_change = 1

    let l:output = system("p4 edit " . expand("%:p"))
    "if v:shell_error == 0
    if stridx(l:output, "not on client") == -1
        set noreadonly
        "echom "File " . expand("%:p") . " opened for edit"
    else
        if g:Confirm("File is not under version control. Do you want to add it?")
            let l:output = system("p4 add " . expand("%:p"))
            if v:shell_error == 0
                set noreadonly
                "echom "File " . expand("%:p") . " opened for add"
            endif
        endif
        echo
    endif

    redraw
    " TODO This doesn't stay in the status line for some weird reason
    " (another redraw sometime later?)
    echom trim(l:output)
endfunction

augroup p4
    autocmd!
    autocmd BufWritePre * :if &readonly | call P4Checkout()
    autocmd FileChangedRO * call P4Checkout()
    autocmd FileChangedShell *
        \ if 1 == s:ignore_change |
        \   let v:fcs_choice="" |
        \   let s:ignore_change=0 |
        \ else |
        \   let v:fcs_choice="ask" |
        \ endif
augroup END

command! P4 call P4Checkout()

"compiler msbuild
" Default errorformat tries to parse a column number which we don't get
"set errorformat=\ %#%f(%l):\ %m
