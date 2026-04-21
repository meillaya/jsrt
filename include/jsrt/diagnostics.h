#ifndef JSRT_DIAGNOSTICS_H
#define JSRT_DIAGNOSTICS_H

char *format_source_diagnostic(
    const char *kind,
    const char *source,
    int line,
    int column,
    int width,
    const char *message
);

#endif
