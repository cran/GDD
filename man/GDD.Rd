\name{GDD}
\alias{GDD}
\title{
  Create a new GD graphics device for creating bitmap files
}
\description{
  \code{GDD} initializes a new GD graphics device.
}
\usage{
GDD(file = "plot", type = "png", width = 400, height = 300, ps = 12, bg = "white")
}
\arguments{
  \item{file}{prefix of the file(s) that will be created. The final
    filename will be of the form
    <file>[<sequence>].<type-extention>. The only case when no extension
    is appended is when the file already has that extension.}
  \item{type}{file format used when producing the output. The actual
    files are created either on close (such as via dev.off()) or when a
    new page is requested. Possible file formats currently include "png",
    "png24", "png8", "gif" and "jpeg".}
  \item{width}{width of the plot area in pixels.}
  \item{height}{height of the plot area in pixels.}
  \item{ps}{initial point size.}
  \item{bg}{plot background. For all types except for "jpeg", you can use
    "transparent" as the packground color. JPEG files don't
    support transparent background. Using non-white, fully transparent
    color can be useful to avoid clashes with existing colors when
    mapping the transparency.}
}
\value{
  The (invisible) return value is NULL if the device couldn't be created
  or the raw device number if successful.
}
\section{Known bugs}{
  \itemize{
    \item Symbols are not supported yet (we need symbol font + mapping
    for this).
    \item Alpha channel is not fully supported (no one complained
    about this, yet).
  }
  Those bugs are rather due to lack of spare time than anything else -
  technically all of them are fairly easy to solve, given
  manpower. Volunteers are encouraged to contact me ;).

  Note: Consider using the \code{Cairo} package instead of \code{GDD},
  it has better rendering, symbol support and the back-end is more
  actively maintained.
} 
\examples{
\dontrun{
GDD(type="jpeg", w=800, h=600)
plot(rnorm(100),rnorm(100))
dev.off() # creates a file "plot.jpeg" with the above plot
}
}
\keyword{device}
