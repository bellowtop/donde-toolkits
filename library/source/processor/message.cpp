#include "Poco/Event.h"
#include "concurrent_processor.h"

#include <iostream>

using Poco::Event;

WorkMessage::WorkMessage(Value request, bool quit_flag) : _request(request), _quit_flag(quit_flag) {
    Event _evt(Event::EVENT_AUTORESET);
    _evt.reset();
}
bool WorkMessage::isQuitMessage() { return _quit_flag; }

void WorkMessage::waitResponse() { _evt.wait(); }

Value WorkMessage::getRequest() { return _request; }
void WorkMessage::setResponse(Value resp) {
    _response = resp;

    _evt.set();
}
Value WorkMessage::getResponse() { return _response; }
