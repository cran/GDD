.First.lib <- function(libname, pkgname) {
  library.dynam("GDD", pkgname, libname)
  .C("gddSetFTFontPath",as.character(paste(libname,pkgname,"fonts","",sep=.Platform$file.sep)),PACKAGE="GDD")
}
