#pragma once
#include "orazio_client.h"

struct OrazioWSContext;

// starts a websocket server biund to the shell
struct OrazioWSContext* OrazioWebsocketServer_start(struct OrazioClient* client,
                                                    int port,
                                                    char* resource_path,
                                                    int rate);

// stops a websocket server biund to the shell
void OrazioWebsocketServer_stop(struct OrazioWSContext* context);

// generates the html stubs for the input captions
void OrazioWebsocketServer_genHtml(char* resource_path);
