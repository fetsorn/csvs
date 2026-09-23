# Nested Records

branches can depend on each other. branch is a name for piece of structure inside a dataset. Imagine that a dataset is a grove of trees, and each tree is made up of branches connected to each other. Branch is called a trunk if it has leaves - branches that describe it. A branch without leaves is called a twig and sits at the very top of the tree. A branch that does not describe any other branch and thus does not have a trunk, is called a root and sits at the very bottom of a tree.

Let's add an `age` to a `name` of a person that experienced an event.

> ``` csv
> # .csvs.csv
> csvs,0.0.2
> ```
> 
> ``` csv
> # _-_.csv
> event,date
> event,name
> name,age
> ```
> 
> ``` csv
> # event-date.csv
> visited Japan,2001-01-01
> climbed Everest,2003-03-03
> ```
> 
> ``` csv
> # event-name.csv
> visited Japan,Donell
> climbed Everest,Eva
> ```
> 
> ``` csv
> # name-age.csv
> Donell,35
> Eva,70
> ```

Now, let's add a favorite quote of each person, and the author of each quote 

> ``` csv
> # .csvs.csv
> csvs,0.0.2
> ```
> 
> ``` csv
> # _-_.csv
> event,date
> event,name
> name,age
> name,quote
> quote,author
> ```
> 
> ``` csv
> # event-date.csv
> visited Japan,2001-01-01
> climbed Everest,2003-03-03
> ```
> 
> ``` csv
> # event-name.csv
> visited Japan,Donell
> climbed Everest,Eva
> ```
> 
> ``` csv
> # name-age.csv
> Donell,35
> Eva,70
> ```
> 
> ``` csv
> # name-quote.csv
> Donell,The only way to do great work is to love what you do
> Eva,"Sometimes you need to scorch everything to the ground, and start over"
> ```
> 
> ``` csv
> # quote-author.csv
> The only way to do great work is to love what you do,Donovan
> "Sometimes you need to scorch everything to the ground, and start over",Celeste Ng
> ```

You can even define a recursive relation to specify the parent of each person

> ``` csv
> # .csvs.csv
> csvs,0.0.2
> ```
> 
> ``` csv
> # _-_.csv
> event,date
> event,name
> name,age
> name,parent
> ```
> 
> ``` csv
> # event-date.csv
> visited Japan,2001-01-01
> climbed Everest,2003-03-03
> ```
> 
> ``` csv
> # event-name.csv
> visited Japan,Donell
> climbed Everest,Eva
> ```
> 
> ``` csv
> # name-parent.csv
> Donell,Jack
> Donell,Jacqueline
> Jack,Rona
> Jack,Bernard
> Jacqueline,Leif
> Jacqueline,Fatuma
> Eva,Ismail
> Eva,Hauwa
> Ismail,Nelson
> Ismail,Dennis
> Hauwa,Rabi
> Hauwa,Louis
> ```

To learn more about csvs, see other [User Guides](./README.md) and the [Requirements](../requirements.md).
