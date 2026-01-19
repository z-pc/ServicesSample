#pragma once

class ApiRouter;

/**
 * @brief Register all application HTTP endpoints on the provided router.
 *
 * This centralizes endpoint registration and keeps `main()`/bootstrap code minimal.
 */
void register_handlers(ApiRouter& api);
