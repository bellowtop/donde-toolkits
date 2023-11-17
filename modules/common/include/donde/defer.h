#pragma once

#define DEFER(statement) std::shared_ptr<bool> defer(nullptr, [&](bool*) { statement; });
