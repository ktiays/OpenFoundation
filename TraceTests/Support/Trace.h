//
//  Created by ktiays on 2026/10/30.
//  Copyright (c) 2026 ktiays. All rights reserved.
//

#pragma once

#include "Support/TraceInterface.h"

#include <stdint.h>

// Verifies the process: the reference build runs on the pinned system CoreFoundation and loads no restored
// framework; the replacement build loads the restored framework and no native Foundation/CoreFoundation.
// Prints the identity as '#' metadata lines. Returns 0, or 77 for a setup failure.
int checkTraceProcess(void);

// Starts a named scenario; object identities and dynamic type IDs restart from 1.
void traceScenario(const char *name);

// One trace line, prefixed with two spaces.
void traceLine(const char *format, ...) __attribute__((format(printf, 1, 2)));

// The first-seen index of an object in this scenario ("@1", "@2", ...), "NULL", or a setup failure when the
// table is full. The string stays valid for the next seven calls of traceObject or traceTypeID.
const char *traceObject(const void *object);

// Removes an object from the identity table after its last release, so a reused address is a new object.
void traceForget(const void *object);

// A type ID: builtin IDs (below 72) as numbers, registered IDs as "dynamic#n" by first appearance.
const char *traceTypeID(unsigned long typeID);

// Whether the reference is a tagged pointer on this architecture (bit 63 set).
int traceIsTagged(const void *object);

// Nonzero when a setup failure occurred while tracing.
int traceSetupFailed(void);
