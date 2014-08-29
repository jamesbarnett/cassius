cassius
=======
<a href-"https://travis-ci.org/jquadrin/spill">![Build Status](https://travis-ci.org/jquadrin/cassius.svg)</a>

#####monitor linux file system events
<br/>
```elixir
Cassius.watch("lib", :all)    # monitor events in a directory
```
<br/>
monitored events are sent to the caller's pid
```elixir
{:open, foobar}               #  {event, file_path} 
```
<br/>
These are the events you can monitor specifically (`:all` monitors all events)

| event			                | defn.                         |
|---------------------------|-------------------------------|
| :close			    			    | file closed                   |
| :access			    			    | file accessed                 |
| :attrib			  			      | file metadata changed         |
| :close_write				    	| file wrote and closed         |
| :close_no_write			      | file closed without write     |
| :create				    		    | file/directory created        |
| :delete						        | file/directory deleted        |
| :delete_self			    		| file/directory itself deleted |
| :modify			  			      | file modified                 |
| :move_self					      | file/directory itself moved   |
| :moved_from					      | file left dir                 |
| :moved_to					        | file entered dir              |
| :open			  			        | file opened                   |
