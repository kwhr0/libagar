with Agar;
with Agar.Event;

package myeventhandler is
  package EV renames Agar.Event;
  
  procedure Ping
    (Event : EV.Event_Access_t)
    with Convention => C;

  procedure Some_Event
    (Event : EV.Event_Access_t)
    with Convention => C;

end myeventhandler;
