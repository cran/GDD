GDD <- function(file="plot", type="png", width=400, height=300, ps=12, bg="white") {
  invisible(.External("gdd_create_new_device", as.character(type), as.character(file), width, height, ps, bg, PACKAGE="GDD"))
}

.GDD.font <- function(name=NULL) {
  .Call("gdd_look_up_font", name, PACKAGE="GDD")
}
