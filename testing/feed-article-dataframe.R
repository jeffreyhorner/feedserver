url <- 'http://0.0.0.0:8000/'
uri <- function(route) paste0(url,route)
user_feed <- dir('articles','rhelp.user')
for (i in user_feed){
  user <- str_split(i,'\\.',simplify=TRUE)[3]
  payload <- file.path('articles',i)
  wget <- 
    paste0("wget -O - --method=PUT --body-file=",payload," ",uri('subscribeuser'))
  system(wget)
}
feed_article <- dir('articles','rhelp.article')
for (i in feed_article){
  feed <- str_split(i,'\\.',simplify=TRUE)[3]
  payload <- file.path('articles',i)
  wget <- paste0("wget -O - --post-file=",payload," ",uri(paste0('feed/',feed)))
  system(wget)
}
system(paste0("wget ",uri('shutdown')))
