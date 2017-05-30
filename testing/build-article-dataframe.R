library(utils)
library(dplyr)
library(stringi)
library(jsonlite)

system("mkdir articles")

sink('/dev/null')
invisible(
  lapply(
    rownames(installed.packages()),
    function(pkg) try(silent=TRUE,library(pkg,character.only=TRUE))
  )
)
sink()

clean_help_file <- function(pkg, fn){
  sink('/dev/null')
  rd <- 
    try(
      tools:::fetchRdDB(file.path(find.package(pkg), "help", pkg),key=fn),
      silent=TRUE
    )
  sink()
  if (is(rd,'try-error')) return(NA)
  capture.output(
    rd %>%
    tools::Rd2txt()
  ) %>%
  paste(collapse="\n") %>%
  stri_replace_all_charclass(pattern="[\\n]",replacement="\\n") %>%
  stri_replace_all_charclass(pattern="\\P{PRINT}",replacement="",merge=TRUE) %>%
  stri_replace_all_charclass(pattern="\\p{QUOTATION_MARK}",replacement="",merge=TRUE)
}

clean_help_file_dplyr <- function(pkg, fn){
  unclass(lapply(seq_along(pkg),function(i) clean_help_file(pkg[i],fn[i]) ) )
}

all_keys <- 
  unclass(hsearch_db(rebuild=TRUE))$Keywords %>% dplyr::select(Keyword,Package) %>%
  dplyr::select(Keyword) %>% .[[1]] %>% unique()

d <- 
  bind_rows(
    lapply(all_keys,function(key){
      unclass(help.search(keyword=key))$matches %>%
        transmute(
          keyword=key,
          package=Package,
          name=Name,
          help=clean_help_file_dplyr(Package,Name)
        )
    })
  ) %>%
  filter(!is.na(help))

user_feed <- d %>% dplyr::select(keyword,package) %>% unique()

feed_article <- 
  d %>% 
  dplyr::select(package,name,help) %>% 
  unique()

lapply(
  user_feed %>% dplyr::select(keyword) %>% .[[1]] %>% unique(),
  function(key){
    write(
      toJSON(
        list(
          `user`=key,
          `feeds`= user_feed %>% filter(keyword==key) %>% 
                   dplyr::select(package) %>% .[[1]]
        )
      ),
      file=paste0('articles/rhelp.user.',key,'.json')
    )
  }
)
lapply(
  feed_article %>% dplyr::select(package) %>% .[[1]] %>% unique(),
  function(pkg){
    write(
      toJSON(
        list(
          `articles` =
            lapply(
              feed_article %>% filter(package==pkg) %>% dplyr::select(help) %>% .[[1]],
              function(hlp) list(`content`=hlp)
            )
        )
      ),
      file=paste0('articles/rhelp.article.',pkg,'.json')
    )
  }
)
