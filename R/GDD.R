GDD <- function(file="plot", type="png", w=400, h=300, ps=12, bg="white") {
  invisible(.External("gdd_create_new_device", as.character(type), as.character(file), w, h, ps, bg, PACKAGE="GDD"))
}
