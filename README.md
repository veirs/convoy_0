# convoy_0
A QT program to model underwater sounds from commercial shipping

This program reads a data file of ship source levels(./shipSourceLevels.df one directory above the source code) and then passes the ships by a listening station and calculates the received level from what ever ships are within acoustic range and averages these numbers on a yearly basis. The output of the model is either the percentage of the time that the Received Level is less than any particular value from the quietest times through the loudest times.  The cumulative percentage can be displayed if so desired.

Results calculated can be saved to disk in a format that R and Execl can easliy read.

For questions of comments, contact vveirs@coloradocollege.edu

October 1, 2017
