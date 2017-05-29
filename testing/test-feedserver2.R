library(httr)
library(jsonlite)
library(magrittr)
library(dplyr)

# Not a reproducible test since installed R packages will
# differ across computers

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

desc_to_article <- function(package){
  paste(
    capture.output(
      packageDescription(package)
    ),
    collapse="\n"
  )
}

package_list <- rownames(installed.packages())

for (i in package_list){
  fareq <- list(`articles`=list(list(`content`=desc_to_article(i)),list(`content`='article2')))
  ret <- POST(uri('feed/feed1'),body=toJSON(fareq))
  print(ret)
}

req <- list(`user`='user1', `feeds`='feed1')

# Subscribe user1 -> feed1
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

