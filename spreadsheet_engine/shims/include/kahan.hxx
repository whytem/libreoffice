#pragma once

// Shim: redirect to the engine's canonical KahanSum.
#include <spreadsheetengine/runtime/KahanSum.hxx>

using KahanSum = spreadsheetengine::core::fp::KahanSum;
