library(httr)
library(jsonlite)

system("rm -rf /tmp/rocksdb_*")
system("../build/feedserver 2>/dev/null >/dev/null &")

Sys.sleep(3)

feedserver_url = 'http://0.0.0.0:8000'
uri <- function(action){
  paste0(feedserver_url,'/',action)
}

shutdown_server <- function(){
  GET(uri('shutdown'))
}

req <- list(`user`='user1', `feeds`='feed1')

# Subscribe user1 -> feed1
ret <- PUT(uri('subscribeuser'),body=toJSON(req))
print(ret)
content(ret)

# Subscribe user1 -> feed2
req$feeds <- 'feed2'
ret <- PUT(uri('subscribeuser'),body=toJSON(req))
print(ret)
content(ret)

# Unsubscribe user1 -> feed1
req$feeds <- 'feed1'
ret <- PUT(uri('unsubscribeuser'),body=toJSON(req))
print(ret)
content(ret)

# Unsubscribe user1 -> feed2
req$feeds <- 'feed2'
ret <- PUT(uri('unsubscribeuser'),body=toJSON(req))
print(ret)
content(ret)

# Get user1
ret <- GET(uri('user/user1'))
print(ret)
content(ret)

# Submit feed1 - article1
fareq <- list(`articles`=list(list(`content`='article1'),list(`content`='article2')))
ret <- POST(uri('feed/feed1'),body=toJSON(fareq))
print(ret)
content(ret)

# Submit feed2 - article1
fareq <- list(`articles`=list(list(`content`='lorem ipsum'),list(`content`='blah blah')))
ret <- POST(uri('feed/feed2'),body=toJSON(fareq))
print(ret)
content(ret)

# Subscribe user1 -> feed1
req$feeds <- 'feed1'
ret <- PUT(uri('subscribeuser'),body=toJSON(req))
print(ret)
content(ret)

# Get articles for user1
ret <- GET(uri('userarticles/user1'))
print(ret)
content(ret)

ret <- shutdown_server()
print(ret)
content(ret)
