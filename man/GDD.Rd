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
    is appended is when the file already has that extension. The sequence
    number is increased with each new page.}
  \item{type}{file format used when producing the output. The actual
    files are created either on close (such as via dev.off()) or when a
    new page is requested. Possible file formats currently include "png",
    "png24", "png8", "gif" and "jpeg".}
  \item{width}{width of the plot area in pixels.}
  \item{height}{height of the plot area in pixels.}
  \item{ps}{initial point size.}
  \item{bg}{plot background. For all types except for "jpeg", you can use
    the string "transparent" as the packground color (JPEG files do not
    support transparent background).}
}
\value{
  The (invisible) return value is NULL if the device couldn't be created
  or the raw device number if successful.
}
\section{Known issues}{
  \itemize{
    \item In the current implementation \code{bg} argument currently
    supports only "transparent" and "white". Any other color is
    silently treated as "white". Moreover transparency is achieved by
    mapping "white" as the transparent color, thus the results may not
    be exactly as expected.
    \item Symbols are supported only if the symbol.ttf font (or
    equivalent) is installed and registered in basefont.mapping
    (should work out of the box in Windows).
    \item Alpha channel is not fully supported.
    about this, yet).
  }
  Those issues are unsolved rather due to lack of spare time than
  anything else - technically all of them are fairly easy to solve,
  given manpower. Volunteers are encouraged to contact me ;).
} 
\seealso{
}
\examples{
GDD("demo.jpeg", type="jpeg", width=800, height=600)
plot(rnorm(100),rnorm(100),col=2)
dev.off() # creates a file "plot.jpeg" with the above plot
}
\keyword{device}
