proc boot { } {
    expect {
        "sched init finished" { }
        timeout { error "schd init failed" }
    }
    expect {
        "create initial thread done" { }
        timeout { error "create init_thread failed" }
    }
}

proc print {msg} {
    puts "\nCI: $msg"
    flush stdout
}

set this_dir [file dirname [file normalize $argv0]]
print "Test script directory: $this_dir"

proc read_test_timeout {config_file} {
    set config_fp [open $config_file r]
    set cases [split [read $config_fp] "\n"]
    close $config_fp
    set timeout_list {}
    foreach case $cases {
        if {[regexp {^\s*$} $case]} {
            continue
        }

        if {[regexp {^#+.*$} $case]} {
            continue
        }

        set part [split $case " "]
        set time [lindex $part 0]
        lappend timeout_list $time
    }
    return $timeout_list
}
