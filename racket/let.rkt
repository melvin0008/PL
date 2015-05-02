#lang racket

(define (max-list xs)
  (cond [(null? xs)(error "No elements in the list")]
        [(null? (cdr xs))(car xs)]
        [#t (let ([ans (max-list (cdr xs))])
              (if (> ans (car xs))
                  ans
                  (car xs)))
                     ]))

(define (test-let x)
  (let ([x (+ x 3)]
        [y (+ x 4)])
        (+ x y)))

(define (swap x y)
  (let ([x y]
        [y x])
            x))