.First.lib <- function(libname, pkgname) {
  if (.Platform$OS.type=="windows") {
    lp<-gsub("/","\\\\",paste(libname,pkgname,"libs",sep="/"))
    cp<-strsplit(Sys.getenv("PATH"),";")
    if (! lp %in% cp) Sys.putenv(PATH=paste(lp,Sys.getenv("PATH"),sep=";"))
  }
  library.dynam("GDD", pkgname, libname)
  .C("gddSetFTFontPath",as.character(paste(libname,pkgname,"fonts","",sep=.Platform$file.sep)),PACKAGE="GDD")
}
